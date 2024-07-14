int putchar(char ch) {
//  If usermode, use syscalls etc.
#ifdef __is_libc

#endif

	return ch;
}