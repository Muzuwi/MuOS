#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H
 
//  TODO: sysdefs

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif