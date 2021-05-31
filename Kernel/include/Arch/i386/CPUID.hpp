#pragma once

namespace CPUID {
	bool has_huge_pages();
	bool has_NXE();
	bool has_SEP();
	bool has_RDRAND();
	bool has_LAPIC();
}