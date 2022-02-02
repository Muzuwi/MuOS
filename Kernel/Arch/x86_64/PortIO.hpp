#pragma once
#include <Kernel/SystemTypes.hpp>

namespace Ports {
	inline void out(uint16 port, uint8 value) {
		asm volatile("mov %%al, %0\n"
		             "mov %%dx, %1\n"
		             "out %%dx, %%al\t\n"
		:
		:"r"(value), "r"(port)
		: "eax", "edx");
	}

	inline void outw(uint16 port, uint16 value) {
		asm volatile("mov %%ax, %0\n"
		             "mov %%dx, %1\n"
		             "out %%dx, %%ax\t\n"
		:
		:"r"(value), "r"(port)
		: "eax", "edx");
	}

	inline void outd(uint16 port, uint32 value) {
		asm volatile("mov %%eax, %0\n"
		             "mov %%dx, %1\n"
		             "out %%dx, %%eax\t\n"
		:
		:"r"(value), "r"(port)
		: "eax", "edx");
	}

	inline uint8 in(uint16 port) {
		uint32 data = 0;
		asm volatile("mov %%dx, %1\n"
		             "in %%al, %%dx\n"
		             "mov %0, %%eax\t\n"
		: "=r"(data)
		: "r"(port)
		: "eax", "edx");
		return data;
	}

	inline uint16 inw(uint16 port) {
		uint16_t data = 0;
		asm volatile("mov %%dx, %1\n"
		             "in %%ax, %%dx\n"
		             "mov %0, %%ax\t\n"
		: "=r"(data)
		: "r"(port)
		: "eax", "edx", "memory");
		return data;
	}

	inline uint32 ind(uint16 port) {
		uint32 data = 0;
		asm volatile("mov %%dx, %1\n"
		             "in %%eax, %%dx\n"
		             "mov %0, %%eax\t\n"
		: "=r"(data)
		: "r"(port)
		: "eax", "edx", "memory");
		return data;

	}
}

void wrmsr(uint32_t ecx, uint64_t value);
uint64_t rdmsr(uint32_t ecx);

