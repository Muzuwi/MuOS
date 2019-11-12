#include <string.h>

void* memchr(const void* ptr, int val, size_t num){
	for(size_t i = 0; i < num; i++){
		if(((unsigned char*)ptr)[i] == (unsigned char)val){
			unsigned char* addr = &((unsigned char*)ptr)[i];
			return (void*)addr;
		}
	}
	return NULL;
}