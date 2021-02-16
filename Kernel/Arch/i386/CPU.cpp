#include <cpuid.h>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/SystemTypes.hpp>
#include <Arch/i386/CPU.hpp>
#include <Arch/i386/CPUID.hpp>

void CPU::initialize_features() {
	uint32_t new_efer {0x100}; //  Long-Mode enable

	kdebugf("[CPU] Support ");
	if(CPUID::has_NXE()) {
		kdebugf("NXE ");
		new_efer |= (1u << 11);
	}
	if(CPUID::has_SEP()) {
		kdebugf("SEP ");
		new_efer |= 1;
	}
	kdebugf("\n");

	asm volatile("wrmsr"
	:: "a"(new_efer), "d"(0), "c"(0xC0000080));
}
