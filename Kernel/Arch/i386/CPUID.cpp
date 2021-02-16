#include <cpuid.h>
#include <Arch/i386/CPUID.hpp>
#include <Kernel/SystemTypes.hpp>

bool CPUID::has_huge_pages() {
	unsigned int eax, _unused, edx;
	__get_cpuid(0x80000001, &eax, &_unused, &_unused, &edx);
	return edx & (1u << 26u);
}

bool CPUID::has_NXE() {
	unsigned int eax, _unused, edx;
	__get_cpuid(0x80000001, &eax, &_unused, &_unused, &edx);
	return edx & (1u << 20u);
}

bool CPUID::has_SEP() {
	unsigned int eax, _unused, edx;
	__get_cpuid(0x1, &eax, &_unused, &_unused, &edx);
	return edx & (1u << 11u);
}
