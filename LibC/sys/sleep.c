#include <sys/syscall.h>
#include <unistd.h>
#include <include/asm/unistd.h>

unsigned int sleep(unsigned int milliseconds) {
	int rc = _syscall_generic_1(__SYS_sleep, milliseconds);
	return rc;
}