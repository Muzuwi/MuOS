#include <sys/types.h>
#include <sys/syscall.h>
#include <asm/unistd.h>

static pid_t __pid_cache = 0;

pid_t getpid() {
	if(!__pid_cache) {
		pid_t rc = _syscall_generic_0(__SYS_getpid);
		__pid_cache = rc;
	}

	return __pid_cache;
}