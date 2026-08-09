#define _CRT_SECURE_NO_WARNINGS
#include "fileio.h"
#include <stdio.h>
#include <stdlib.h>

unsigned char* read_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    *out_len = ftell(f);
    rewind(f);

    unsigned char* buf = (unsigned char*)malloc(*out_len);
    fread(buf, 1, *out_len, f);
    fclose(f);
    return buf;
}

void write_file(const char* path, unsigned char* data, size_t len) {
    FILE* f = fopen(path, "wb");
    fwrite(data, 1, len, f);
    fclose(f);
}