#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t strlen(const char* string);
int strcmp(const char*, const char*);

void* memcpy(void*, const void*, size_t);
void* memchr(const void*, int, size_t);
void* memset(void*, int, size_t);
int memcmp(const void*, const void*, size_t);

#endif