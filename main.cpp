#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio.h"
#include "aes.h"

int main(int argc, char* argv[]) {
    if (argc < 5) {
        printf("micrpt - AES-256 file encryptor\n");
        printf("--------------------------------\n");
        printf("Usage:\n");
        printf("  micrpt crypt   <input> <output> <password>\n");
        printf("  micrpt decrypt <input> <output> <password>\n");
        printf("\nExamples:\n");
        printf("  micrpt crypt   1.exe 2.exe mypassword\n");
        printf("  micrpt decrypt 2.exe  3.exe mypassword\n");
        return 1;
    }

    char* action = argv[1];
    char* in_path = argv[2];
    char* out_path = argv[3];
    char* password = argv[4];

    if (strcmp(action, "crypt") != 0 && strcmp(action, "decrypt") != 0) {
        printf("Error: unknown command '%s'\n", action);
        return 1;
    }

    if (strlen(password) < 4) {
        printf("Error: password must be at least 4 characters\n");
        return 1;
    }

    size_t input_len;
    unsigned char* input = read_file(in_path, &input_len);
    if (!input) {
        printf("Error: cannot open '%s'\n", in_path);
        return 1;
    }
    if (input_len == 0) {
        printf("Error: input file is empty\n");
        free(input);
        return 1;
    }

    unsigned char* output = NULL;
    size_t output_len = 0;
    int ok = 0;

    if (strcmp(action, "crypt") == 0) {
        ok = aes_encrypt(input, input_len, password, &output, &output_len);
        if (!ok) printf("Error: encryption failed\n");
    }
    else {
        ok = aes_decrypt(input, input_len, password, &output, &output_len);
        if (!ok) printf("Error: decryption failed (wrong password?)\n");
    }

    free(input);

    if (ok) {
        write_file(out_path, output, output_len);
        free(output);
        if (strcmp(action, "crypt") == 0)
            printf("Encrypted: %s -> %s (%zu -> %zu bytes)\n",
                in_path, out_path, input_len, output_len);
        else
            printf("Decrypted: %s -> %s (%zu -> %zu bytes)\n",
                in_path, out_path, input_len, output_len);
    }

    return ok ? 0 : 1;
}