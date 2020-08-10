#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t write(int, const void*, size_t);

pid_t getpid();
unsigned int sleep(unsigned int ms);

#ifdef __cplusplus
}
#endif

#endif
