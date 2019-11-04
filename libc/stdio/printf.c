#include <stdarg.h>
#include <stddef.h>


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


struct dest_t {
	char* dest_buffer;
	unsigned int dest_buffer_len;
};


int _vsnprintf_internal(const char* format, struct dest_t destination, va_list arg) {
	//  TODO: Bounds checking all over the place

	char buffer[PRINTF_BUFFER_SIZE];

	//  For convenience
	char* dst_bfr = destination.dest_buffer;

	//  Sanity check
	if(destination.dest_buffer_len == 0 || destination.dest_buffer == 0) return 0;

	//  Total amount of characters written to stdout
	unsigned int total_chars_written = 0;
	//  Position in the format buffer
	unsigned int format_cnt = 0;
	//  Position in the destination buffer
	unsigned int dst_cnt = 0;

	while(format[format_cnt] != '\0'){
		//  Ignore any characters that would not fit the destination buffer
		if(dst_cnt >= destination.dest_buffer_len){
			total_chars_written++;
			format_cnt++;
			continue;
		}
		//  Character escape sequence \x
		if(format[format_cnt] == '\\'){
			if(format[format_cnt+1] != '\0'){
				dst_bfr[dst_cnt++] = format[format_cnt+1];
				format_cnt += 2;
				total_chars_written++;
				continue;
			}
		}

		if(format[format_cnt] == '%'){
			//  Escape seq %%
			if(format[format_cnt+1] != '\0' && format[format_cnt+1] == '%'){
				dst_bfr[dst_cnt++] = format[format_cnt];
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
					char ch = (char)va_arg(arg, int);
					dst_bfr[dst_cnt++] = ch;
					total_chars_written++;
					break;
				}
				case 'i':{
					int num = va_arg(arg, int);
					if(num == 0){
						dst_bfr[dst_cnt++] = '0';
						total_chars_written++;
						break;
					}

					int workPointer = 0;
					while(num != 0){
						buffer[workPointer++] = (ascii_lookup_table[47 + (num % 10)]);
						num = num / 10;
					}

					//  Correct pointer to point to last character in buffer
					workPointer--;
					while(workPointer >= 0){
						dst_bfr[dst_cnt++] = buffer[workPointer--];
						total_chars_written++;
					}

					break;
				}
				case 'x':
				case 'X':{
					int num = va_arg(arg, int);
					unsigned int mask = 15;
					int block = sizeof(int)*2 - 1;
					while(block >= 0){
						char ch;
						if(format[format_cnt+1] == 'x'){
							ch = hex_lookup_table[(num >> block*4) & mask];
						} else {
							ch = hex_lookup_table_upper[(num >> block*4) & mask];
						}
						dst_bfr[dst_cnt++] = ch;
						total_chars_written++;
						block--;
					}
					break;
				}
				case 's':{
					char* str = va_arg(arg, char*);
					if(str == 0) break;
					unsigned int ptr = 0;
					while(str[ptr] != '\0'){
						dst_bfr[dst_cnt++] = str[ptr];
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
			dst_bfr[dst_cnt++] = format[format_cnt];
			total_chars_written++;
		}
		format_cnt++;
	}

	//  Null terminate
	dst_bfr[dst_cnt++] = '\0';

	return total_chars_written;
}


/*
	This function takes a format string as argument and prints
	to stdout accordingly
	Returns: amount of characters written
*/
int printf(const char* format, ...){
	char buf[PRINTF_BUFFER_SIZE];

	struct dest_t destination;
	destination.dest_buffer = buf;
	destination.dest_buffer_len = PRINTF_BUFFER_SIZE;

	va_list args;
	
	va_start(args, format);
	int c = _vsnprintf_internal(format, destination, args);
	va_end(args);

	puts(buf);

	return c;
}

int vprintf(const char* format, va_list args){
	char buf[PRINTF_BUFFER_SIZE];

	struct dest_t destination;
	destination.dest_buffer = buf;
	destination.dest_buffer_len = PRINTF_BUFFER_SIZE;

	int c = _vsnprintf_internal(format, destination, args);

	puts(buf);

	return c;
}


int snprintf(char* dest, size_t size, const char* format, ...){
	struct dest_t destination;
	destination.dest_buffer = dest;
	destination.dest_buffer_len = size;

	va_list args;

	va_start(args, format);
	int c = _vsnprintf_internal(format, destination, args);
	va_end(args);

	return c;
}

int vsnprintf(char* dest, size_t size, const char* format, va_list args){
	struct dest_t destination;
	destination.dest_buffer = dest;
	destination.dest_buffer_len = size;

	int c = _vsnprintf_internal(format, destination, args);

	return c;
}