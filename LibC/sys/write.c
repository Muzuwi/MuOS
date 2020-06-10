#include <sys/types.h>
#include <sys/syscall.h>
#include <errno.h>
#include <asm/unistd.h>

ssize_t write(int fd, const void* buf, size_t nbyte) {
	int rc = _syscall_generic_3(__SYS_write, fd, (uint32_t)buf, nbyte);
	if(rc < 0) {
		errno = rc;
		return -1;
	}

	errno = 0;
	return rc;
}