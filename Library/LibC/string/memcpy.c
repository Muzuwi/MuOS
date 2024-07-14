#ifdef __LIBC_DEBUG
#	include <stdio.h>
#endif

#include <string.h>

typedef int word;
typedef unsigned char byte;

void* memcpy(void* dest, const void* src, size_t num) {
	word* dest_addr = (word*)dest;
	word* src_addr = (word*)src;
	unsigned int words = num / sizeof(word), excess_byte = num % sizeof(word);

#ifdef __LIBC_DEBUG
	printf("memcpy: copying %i machine words, %i excess bytes\n", words, excess_byte);
#endif

	for(size_t i = 0; i < words; ++i) {
		dest_addr[i] = src_addr[i];
	}

	if(excess_byte > 0) {
		byte* dest_addr = &((byte*)dest)[num - excess_byte];
		byte* src_addr = &((byte*)src)[num - excess_byte];
		for(size_t i = 0; i < excess_byte; ++i) {
			dest_addr[i] = src_addr[i];
		}
	}

	return dest;
}