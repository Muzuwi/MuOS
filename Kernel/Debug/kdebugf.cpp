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

	int c = vsnprintf(buffer, KDEBUGF_BUFFER_SIZE, format, args);

	va_end(args);

	Serial::write_str(Serial::Port::COM0, "\u001b[1;91m");
	Serial::write_str(Serial::Port::COM0, buffer);
	Serial::write_str(Serial::Port::COM0, "\u001b[0m");

	TTY::prints(buffer);

	return c;
}
