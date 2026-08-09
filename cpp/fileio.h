#ifndef FILEIO_H
#define FILEIO_H

#include <stddef.h>

unsigned char* read_file(const char* path, size_t* out_len);
void write_file(const char* path, unsigned char* data, size_t len);

#endif#pragma once
