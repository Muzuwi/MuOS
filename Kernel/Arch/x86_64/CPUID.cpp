#include <cpuid.h>
#include <Arch/x86_64/CPUID.hpp>
#include <SystemTypes.hpp>

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

bool CPUID::has_RDRAND() {
	unsigned int eax, ecx, _unused, edx;
	__get_cpuid(0x1, &eax, &_unused, &ecx, &edx);
	return ecx & (1u << 30u);
}

bool CPUID::has_LAPIC() {
	unsigned int eax, _unused, edx;
	__get_cpuid(0x1, &eax, &_unused, &_unused, &edx);
	return edx & (1u << 9u);
}
