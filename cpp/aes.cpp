#define _CRT_SECURE_NO_WARNINGS
#include "aes.h"
#include <stdlib.h>
#include <string.h>

#define SALT_SIZE  16
#define IV_SIZE    16
#define KEY_SIZE   32
#define ITERATIONS 100000

// ============================================================================
// РЕАЛИЗАЦИЯ ДЛЯ WINDOWS (WinAPI BCrypt)
// ============================================================================
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

static int derive_key(const char* password, const unsigned char* salt, unsigned char* key_out) {
    BCRYPT_ALG_HANDLE alg = NULL;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG)))
        return 0;

    int ok = BCRYPT_SUCCESS(BCryptDeriveKeyPBKDF2(
        alg, (PUCHAR)password, (ULONG)strlen(password),
        (PUCHAR)salt, SALT_SIZE, ITERATIONS, key_out, KEY_SIZE, 0));

    BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

int aes_encrypt(const unsigned char* input, size_t input_len, const char* password, unsigned char** output, size_t* output_len) {
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char derived[KEY_SIZE];
    ULONG encrypted_len = 0;
    ULONG actual = 0;
    int ok = 0;

    BCryptGenRandom(NULL, salt, SALT_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    BCryptGenRandom(NULL, iv, IV_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (!derive_key(password, salt, derived)) goto cleanup;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0))) goto cleanup;
    if (!BCRYPT_SUCCESS(BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0))) goto cleanup;
    if (!BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(alg, &key, NULL, 0, derived, KEY_SIZE, 0))) goto cleanup;

    BCryptEncrypt(key, (PUCHAR)input, (ULONG)input_len, NULL, iv, IV_SIZE, NULL, 0, &encrypted_len, BCRYPT_BLOCK_PADDING);

    *output_len = SALT_SIZE + IV_SIZE + encrypted_len;
    *output = (unsigned char*)malloc(*output_len);
    if (!*output) goto cleanup;

    memcpy(*output, salt, SALT_SIZE);
    memcpy(*output + SALT_SIZE, iv, IV_SIZE);

    if (!BCRYPT_SUCCESS(BCryptEncrypt(key, (PUCHAR)input, (ULONG)input_len, NULL, iv, IV_SIZE, *output + SALT_SIZE + IV_SIZE, encrypted_len, &actual, BCRYPT_BLOCK_PADDING))) {
        free(*output);
        *output = NULL;
        goto cleanup;
    }
    ok = 1;

cleanup:
    if (key) BCryptDestroyKey(key);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

int aes_decrypt(const unsigned char* input, size_t input_len, const char* password, unsigned char** output, size_t* output_len) {
    if (input_len <= SALT_SIZE + IV_SIZE) return 0;

    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    unsigned char derived[KEY_SIZE];
    unsigned char iv[IV_SIZE];
    ULONG decrypted_len = 0;
    int ok = 0;

    const unsigned char* salt = input;
    const unsigned char* iv_src = input + SALT_SIZE;
    const unsigned char* ciphertext = input + SALT_SIZE + IV_SIZE;
    ULONG cipher_len = (ULONG)(input_len - SALT_SIZE - IV_SIZE);

    memcpy(iv, iv_src, IV_SIZE);

    if (!derive_key(password, salt, derived)) goto cleanup;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0))) goto cleanup;
    if (!BCRYPT_SUCCESS(BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0))) goto cleanup;
    if (!BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(alg, &key, NULL, 0, derived, KEY_SIZE, 0))) goto cleanup;

    *output = (unsigned char*)malloc(cipher_len);
    if (!*output) goto cleanup;

    if (!BCRYPT_SUCCESS(BCryptDecrypt(key, (PUCHAR)ciphertext, cipher_len, NULL, iv, IV_SIZE, *output, cipher_len, &decrypted_len, BCRYPT_BLOCK_PADDING))) {
        free(*output);
        *output = NULL;
        goto cleanup;
    }

    *output_len = decrypted_len;
    ok = 1;

cleanup:
    if (key) BCryptDestroyKey(key);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

// ============================================================================
// РЕАЛИЗАЦИЯ ДЛЯ LINUX / MACOS (OpenSSL)
// ============================================================================
#else
#include <openssl/evp.h>
#include <openssl/rand.h>

static int derive_key(const char* password, const unsigned char* salt, unsigned char* key_out) {
    return PKCS5_PBKDF2_HMAC(password, (int)strlen(password), salt, SALT_SIZE, ITERATIONS, EVP_sha256(), KEY_SIZE, key_out);
}

int aes_encrypt(const unsigned char* input, size_t input_len, const char* password, unsigned char** output, size_t* output_len) {
    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char key[KEY_SIZE];

    if (RAND_bytes(salt, SALT_SIZE) != 1 || RAND_bytes(iv, IV_SIZE) != 1) return 0;
    if (!derive_key(password, salt, key)) return 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    size_t max_out_len = SALT_SIZE + IV_SIZE + input_len + EVP_MAX_BLOCK_LENGTH;
    *output = (unsigned char*)malloc(max_out_len);
    if (!*output) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    memcpy(*output, salt, SALT_SIZE);
    memcpy(*output + SALT_SIZE, iv, IV_SIZE);

    int len = 0, ciphertext_len = 0;
    unsigned char* cipher_ptr = *output + SALT_SIZE + IV_SIZE;

    if (EVP_EncryptUpdate(ctx, cipher_ptr, &len, input, (int)input_len) != 1) goto err;
    ciphertext_len += len;

    if (EVP_EncryptFinal_ex(ctx, cipher_ptr + len, &len) != 1) goto err;
    ciphertext_len += len;

    *output_len = SALT_SIZE + IV_SIZE + ciphertext_len;
    EVP_CIPHER_CTX_free(ctx);
    return 1;

err:
    free(*output);
    *output = NULL;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

int aes_decrypt(const unsigned char* input, size_t input_len, const char* password, unsigned char** output, size_t* output_len) {
    if (input_len <= SALT_SIZE + IV_SIZE) return 0;

    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];

    const unsigned char* salt = input;
    const unsigned char* iv_src = input + SALT_SIZE;
    const unsigned char* ciphertext = input + SALT_SIZE + IV_SIZE;
    size_t cipher_len = input_len - SALT_SIZE - IV_SIZE;

    memcpy(iv, iv_src, IV_SIZE);

    if (!derive_key(password, salt, key)) return 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    *output = (unsigned char*)malloc(cipher_len);
    if (!*output) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    int len = 0, plaintext_len = 0;

    if (EVP_DecryptUpdate(ctx, *output, &len, ciphertext, (int)cipher_len) != 1) goto err;
    plaintext_len += len;

    if (EVP_DecryptFinal_ex(ctx, *output + len, &len) != 1) goto err;
    plaintext_len += len;

    *output_len = plaintext_len;
    EVP_CIPHER_CTX_free(ctx);
    return 1;

err:
    free(*output);
    *output = NULL;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}
#endif