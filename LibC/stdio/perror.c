#include <asm/errno.h>
#include <stdio.h>
#include <stdint.h>

extern int32_t errno;

void perror(const char* str) {
	if(errno >= 0) return;

	char buffer[256];
	buffer[255] = '\0';

	switch (errno) {
		case 0:
			snprintf(buffer, 256, "%s: OK", str);
			break;
		case -EPERM:
			snprintf(buffer, 256, "%s: Bad Address", str);
			break;
		case -EBADF:
			snprintf(buffer, 256, "%s: Bad file descriptor", str);
			break;
		case -EFBIG:
			snprintf(buffer, 256, "%s: Bad size", str);
			break;
		default:
			snprintf(buffer, 256, "%s: <unknown error code>", str);
	}

	printf(buffer);
}