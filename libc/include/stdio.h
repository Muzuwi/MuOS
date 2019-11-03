#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H
 
//  TODO: sysdefs
#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict, ...);
int vprintf(const char* __restrict, va_list);
int snprintf(char*, size_t, const char*, ...);
int vsnprintf(char*, size_t, const char*, va_list);
int putchar(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif