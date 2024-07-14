#include <string.h>

typedef int word;
typedef unsigned char byte;

//  TODO: Optimize?
void* memset(void* ptr, int val, size_t num) {
	unsigned char setval = (unsigned char)val;
	unsigned char* dest = (unsigned char*)ptr;
	for(size_t i = 0; i < num; ++i) {
		dest[i] = setval;
	}
	return ptr;
}