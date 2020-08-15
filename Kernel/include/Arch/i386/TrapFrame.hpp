#pragma once
#include <Kernel/SystemTypes.hpp>

struct TrapFrame {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t handler_esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t eip;
	uint32_t CS;
	uint32_t EFLAGS;
	uint32_t _user_esp;
	uint32_t _user_SS;
}__attribute__((packed));

struct ErrorCodeTrapFrame {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t handler_esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t error_code;
	uint32_t eip;
	uint32_t CS;
	uint32_t EFLAGS;
	uint32_t _user_esp;
	uint32_t _user_SS;
};
