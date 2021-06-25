#ifndef __LIBC_STRING_H
# define __LIBC_STRING_H

# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>

# ifdef __cplusplus
extern "C" {
# endif

#ifndef NULL
#define NULL 0
#endif

size_t strlen(const char*);
int strcmp(const char*, const char*);
int strncmp(const char* str1, const char* str2, size_t num);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);

void* memcpy(void*, const void*, size_t);
void* memchr(const void*, int, size_t);
void* memset(void*, int, size_t);
int memcmp(const void*, const void*, size_t);

# ifdef __cplusplus
}
# endif

#endif