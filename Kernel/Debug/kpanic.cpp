#include <Debug/kpanic.hpp>
#include <Debug/klogf.hpp>

[[noreturn]] void _kpanic_internal(const char* file, int line){
	klogf_static("!!! Kernel panic at '{}', line {}\n", file, (int)line);
	klogf_static("!!! System halted\n");
	while(true)
		asm volatile("cli\nhlt");
}