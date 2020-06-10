#ifndef __SYSCALL_H
#define __SYSCALL_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t _syscall_generic(uint32_t func_id,
		uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

uint32_t _syscall_generic_4(uint32_t func_id,
        uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

uint32_t _syscall_generic_3(uint32_t func_id,
        uint32_t arg1, uint32_t arg2, uint32_t arg3);

uint32_t _syscall_generic_2(uint32_t func_id,
        uint32_t arg1, uint32_t arg2);

uint32_t _syscall_generic_1(uint32_t func_id,
        uint32_t arg1);

#ifdef __cplusplus
};
#endif

#endif