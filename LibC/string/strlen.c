#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t strlen(const char* str) {
	size_t count = 0;
	while(str[count])
		count++;

	return count;
}
