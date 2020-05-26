#include <Arch/i386/CPU.hpp>

void CPU::initialize_features() {
	asm volatile(
	"wrmsr"
	:: "a"(1), "d"(0), "c"(0xC0000080)
	);
}