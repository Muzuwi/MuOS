#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/TTY.hpp>
#include <Kernel/Device/Serial.hpp>
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

//	Ports::out(0x3f8, '\u001b');
//	Ports::out(0x3f8, '[');
//	Ports::out(0x3f8, '3');
//	Ports::out(0x3f8, '2');
//	Ports::out(0x3f8, 'm');

	//  TODO: Maybe don't use strlen here?
//	for(size_t i = 0; i < strlen(buffer); i++){
//		Ports::out(0x3f8, buffer[i]);
//	}


//	Ports::out(0x3f8, '\u001b');
//	Ports::out(0x3f8, '[');
//	Ports::out(0x3f8, '0');
//	Ports::out(0x3f8, 'm');

	Serial::write_str(Serial::Port::COM0, "\u001b[32m");
	Serial::write_str(Serial::Port::COM0, buffer);
	Serial::write_str(Serial::Port::COM0, "\u001b[0m");

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

//	Ports::out(0x3f8, '\u001b');
//	Ports::out(0x3f8, '[');
//	Ports::out(0x3f8, '1');
//	Ports::out(0x3f8, ';');
//	Ports::out(0x3f8, '9');
//	Ports::out(0x3f8, '1');
//	Ports::out(0x3f8, 'm');
//
//
//	for(size_t i = 0; i < strlen(buffer); i++){
//		Ports::out(0x3f8, buffer[i]);
//	}
//
//	Ports::out(0x3f8, '\u001b');
//	Ports::out(0x3f8, '[');
//	Ports::out(0x3f8, '0');
//	Ports::out(0x3f8, 'm');

	Serial::write_str(Serial::Port::COM0, "\u001b[1;91m");
	Serial::write_str(Serial::Port::COM0, buffer);
	Serial::write_str(Serial::Port::COM0, "\u001b[0m");

	TTY::prints(buffer);


	return c;
}
