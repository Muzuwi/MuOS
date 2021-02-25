#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/TTY.hpp>
#include <Arch/i386/PortIO.hpp>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define KDEBUGF_BUFFER_SIZE 512

int kdebugf(const char* format, ...){
	char buffer[KDEBUGF_BUFFER_SIZE];

	va_list args;
	va_start(args, format);

	int c = vsnprintf(buffer, KDEBUGF_BUFFER_SIZE, format, args);

	va_end(args);

	out(0x3f8, '\u001b');
	out(0x3f8, '[');
	out(0x3f8, '3');
	out(0x3f8, '2');
	out(0x3f8, 'm');

	//  TODO: Maybe don't use strlen here?
	for(size_t i = 0; i < strlen(buffer); i++){
		out(0x3f8, buffer[i]);
	}

	out(0x3f8, '\u001b');
	out(0x3f8, '[');
	out(0x3f8, '0');
	out(0x3f8, 'm');

	TTY::prints(buffer);

	return c;
}

int kerrorf(const char* format, ...){
	char buffer[KDEBUGF_BUFFER_SIZE];

	va_list args;
	va_start(args, format);

	//  TODO: Maybe don't use strlen here?
	int c = vsnprintf(buffer, KDEBUGF_BUFFER_SIZE, format, args);

	va_end(args);

	out(0x3f8, '\u001b');
	out(0x3f8, '[');
	out(0x3f8, '1');
	out(0x3f8, ';');
	out(0x3f8, '9');
	out(0x3f8, '1');
	out(0x3f8, 'm');


	for(size_t i = 0; i < strlen(buffer); i++){
		out(0x3f8, buffer[i]);
	}

	out(0x3f8, '\u001b');
	out(0x3f8, '[');
	out(0x3f8, '0');
	out(0x3f8, 'm');

	TTY::prints(buffer);


	return c;
}
