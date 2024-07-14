#ifndef __LIBC_STDIO_H
#define __LIBC_STDIO_H

#include <stdarg.h>
#include <string.h>

//  FIXME:  Dummy
struct __FILE_struct {
	unsigned _dummy;
};

//  FIXME:  Dummy
typedef struct __FILE_struct FILE;

//  FIXME:  Dummy
extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#ifdef __cplusplus
extern "C" {
#endif

	int printf(const char* __restrict, ...);
	int vprintf(const char* __restrict, va_list);
	int snprintf(char*, size_t, const char*, ...);
	int vsnprintf(char*, size_t, const char*, va_list);
	int putchar(int);
	int puts(const char*);
	void perror(const char* str);

	/*
	 *  File operations
	 *  FIXME: Dummies
	 */
	int fclose(FILE* stream);
	FILE* fopen(const char* pathname, const char* mode);
	size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
	size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
	int fseek(FILE* stream, long offset, int whence);
	long ftell(FILE*);
	void setbuf(FILE* stream, char* buf);

	int vfprintf(FILE* stream, const char* format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif