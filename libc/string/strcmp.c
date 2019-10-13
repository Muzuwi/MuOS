#include <string.h>

int strcmp(const char* str1, const char* str2){
	unsigned int ptr = 0;
	do{
		if(str1[ptr] < str2[ptr]) return -1;
		else if(str1[ptr] > str2[ptr]) return 1;

		ptr++;
	}while(str1[ptr-1] != '\0' && str2[ptr-1] != '\0');

	//  FIXME: Is this correct?
	return 0;
}