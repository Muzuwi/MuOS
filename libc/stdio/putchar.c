#ifdef __is_libkernel
#include <kernel/tty.h>
#endif

int putchar(char ch){

	//  If libkernel, call tty directly
	#ifdef __is_libkernel
	tty_printch(ch);
	#endif

	//  If usermode, use syscalls etc.
	#ifdef __is_libc

	#endif

	return ch;
}