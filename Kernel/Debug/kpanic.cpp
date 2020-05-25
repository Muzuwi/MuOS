#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Debug/kdebugf.hpp>

[[noreturn]] void _kpanic_internal(const char* file, int line){
	kerrorf("!!! Kernel panic at '%s', line %i\n", file, (int)line);
	kerrorf("!!! System halted\n");
	asm volatile("cli\nhlt");
	while(1);
}