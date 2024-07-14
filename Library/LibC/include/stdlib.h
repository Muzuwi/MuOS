#ifndef __LIBC_STDLIB_H
#define __LIBC_STDLIB_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

	void abort(void);          //  FIXME: Dummy
	void free(void*);          //  FIXME: Dummy
	void* malloc(size_t);      //  FIXME: Dummy
	int atexit(void (*)(void));//  FIXME: Dummy

	int atoi(const char*);//  FIXME: Dummy

	char* getenv(const char*);//  FIXME: Dummy

#ifdef __cplusplus
}
#endif

#endif
