#ifdef __LIBC_DEBUG
#	include <stdio.h>
#endif

#include <string.h>

typedef int word;
typedef unsigned char byte;

int memcmp(const void* src1, const void* src2, size_t num) {
	word* src1_addr = (word*)src1;
	word* src2_addr = (word*)src2;
	unsigned int words = num / sizeof(word), excess_bytes = num % sizeof(word);

#ifdef __LIBC_DEBUG
	printf("memcmp: comparing %i machine words, %i excess bytes\n", words, excess_bytes);
#endif

	for(size_t i = 0; i < words; ++i) {
		if(src1_addr[i] < src2_addr[i])
			return -1;
		else if(src1_addr[i] > src2_addr[i])
			return 1;
	}

	if(excess_bytes > 0) {
		byte* src1_addr = &((byte*)src1)[num - excess_bytes];
		byte* src2_addr = &((byte*)src2)[num - excess_bytes];
		for(size_t i = 0; i < excess_bytes; ++i) {
			if(src1_addr[i] < src2_addr[i])
				return -1;
			else if(src1_addr[i] > src2_addr[i])
				return 1;
		}
	}

	//  FIXME: Is this actually correct?
	return 0;
}