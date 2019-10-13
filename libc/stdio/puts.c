#include <stdio.h>

int puts(const char* ch){
	unsigned int ptr = 0;
	while(ch[ptr] != '\0'){
		putchar(ch[ptr]);
		ptr++;
	}
}