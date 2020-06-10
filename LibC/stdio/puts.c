#include <stdio.h>
#include <string.h>
#include <unistd.h>

int puts(const char* ch){
	if(write(0, ch, strlen(ch)) < 0)
		return EOF;
	else
		return 1;
}