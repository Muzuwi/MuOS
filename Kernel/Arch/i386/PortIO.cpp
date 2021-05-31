#include <Arch/i386/PortIO.hpp>

void wrmsr(uint32_t ecx, uint64_t value) {
	uint32_t eax = value&0xffffffffu,
			 edx = value>>32u;
	asm volatile("wrmsr"
	:
	:"c"(ecx), "a"(eax), "d"(edx));
}

uint64_t rdmsr(uint32_t ecx) {
	uint32_t eax = 0, edx = 0;
	asm volatile("rdmsr"
	: "=a"(eax), "=d"(edx)
	: "c"(ecx));
	return (static_cast<uint64_t>(edx)<<32u) | (static_cast<uint64_t>(eax));
}
