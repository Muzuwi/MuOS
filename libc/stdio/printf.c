#include <stdarg.h>


#define PRINTF_BUFFER_SIZE 1024

const char ascii_lookup_table[256] = {
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '0', 
'1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@', 
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_', '`', 
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 
'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', ' ', ' ', 
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
};

const char hex_lookup_table[16] = {
	'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};

const char hex_lookup_table_upper[16] = {
	'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};


/*
	This function takes a format string as argument and prints
	to stdout accordingly
	Returns: amount of characters written
*/
int printf(const char* format, ...){
	char buffer[PRINTF_BUFFER_SIZE];

	va_list args;
	va_start(args, format);

	//  Total amount of characters written to stdout
	unsigned int total_chars_written = 0;

	unsigned int format_cnt = 0;
	while(format[format_cnt] != '\0'){
		//  Character escape sequence \x
		if(format[format_cnt] == '\\'){
			if(format[format_cnt+1] != '\0'){
				putchar(format[format_cnt+1]);
				format_cnt += 2;
				total_chars_written++;
				continue;
			}
		}

		if(format[format_cnt] == '%'){
			//  Escape seq %%
			if(format[format_cnt+1] != '\0' && format[format_cnt+1] == '%'){
				putchar(format[format_cnt]);
				format_cnt += 2;
				total_chars_written++;
				continue;
			}

			//  Unterminated format
			if(format[format_cnt+1] == '\0'){
				break;
			}

			switch(format[format_cnt+1]){
				case 'c': {
					char ch = (char)va_arg(args, int);
					putchar(ch);
					total_chars_written++;
					break;
				}
				case 'i':{
					int num = va_arg(args, int);

					int workPointer = 0;
					while(num != 0){
						buffer[workPointer++] = (ascii_lookup_table[47 + (num % 10)]);
						num = num / 10;
					}

					//  Correct pointer to point to last character in buffer
					workPointer--;
					while(workPointer >= 0){
						putchar(buffer[workPointer--]);
						total_chars_written++;
					}

					break;
				}
				case 'x':
				case 'X':{
					int num = va_arg(args, int);
					unsigned int mask = 15;
					int block = sizeof(int)*2 - 1;
					while(block >= 0){
						char ch;
						if(format[format_cnt+1] == 'x'){
							ch = hex_lookup_table[(num >> block*4) & mask];
						} else {
							ch = hex_lookup_table_upper[(num >> block*4) & mask];
						}
						putchar(ch);
						total_chars_written++;
						block--;
					}
					break;
				}
				case 's':{
					char* str = va_arg(args, char*);
					unsigned int ptr = 0;
					while(str[ptr] != '\0'){
						putchar(str[ptr]);	
						total_chars_written++;
						ptr++;
					} 
					break;
				}
				default:
					break;
			}
			format_cnt++;
		} else {
			putchar(format[format_cnt]);
			total_chars_written++;
		}
		format_cnt++;
	}

	va_end(args);

	return total_chars_written;
}
