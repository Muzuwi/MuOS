#pragma once

#include <stddef.h>
#include <stdint.h>

namespace mem {

	static inline void* memset(void* ptr, int val, size_t num) {
		auto setval = (unsigned char)val;
		auto* dest = (unsigned char*)ptr;
		for(size_t i = 0; i < num; ++i) {
			dest[i] = setval;
		}
		return ptr;
	}

	static inline void* memcpy(void* to, void const* from, size_t num) {
		auto* dest = (unsigned char*)to;
		auto* src = (unsigned char*)from;
		for(size_t i = 0; i < num; ++i) {
			dest[i] = src[i];
		}
		return to;
	}

	static inline size_t strcmp(const char* str1, const char* str2) {
		unsigned int ptr = 0;
		do {
			if(str1[ptr] < str2[ptr])
				return -1;
			else if(str1[ptr] > str2[ptr])
				return 1;

			ptr++;
		} while(str1[ptr - 1] != '\0' && str2[ptr - 1] != '\0');

		return 0;
	}

}