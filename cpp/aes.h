#ifndef AES_H
#define AES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int aes_encrypt(const unsigned char* input, size_t input_len,
    const char* password,
    unsigned char** output, size_t* output_len);

int aes_decrypt(const unsigned char* input, size_t input_len,
    const char* password,
    unsigned char** output, size_t* output_len);

#ifdef __cplusplus
}
#endif

#endif