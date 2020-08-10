#include <sys/syscall.h>

int32_t errno = 0;

uint32_t _syscall_generic(uint32_t func_id,
                          uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
	uint32_t _retval = 0;

	asm volatile(
	"int 0x80\n"
	:"=a"(_retval)
	:"a"(func_id),
	"b"(arg1),"c"(arg2),"d"(arg3),"S"(arg4),"D"(arg5)
	);

	return _retval;
}

uint32_t _syscall_generic_4(uint32_t func_id,
                            uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
	return _syscall_generic(func_id, arg1, arg2, arg3, arg4, 0);
}

uint32_t _syscall_generic_3(uint32_t func_id,
                            uint32_t arg1, uint32_t arg2, uint32_t arg3) {
	return _syscall_generic(func_id, arg1, arg2, arg3, 0, 0);
}

uint32_t _syscall_generic_2(uint32_t func_id,
                            uint32_t arg1, uint32_t arg2) {
	return _syscall_generic(func_id, arg1, arg2, 0, 0, 0);
}

uint32_t _syscall_generic_1(uint32_t func_id,
                            uint32_t arg1) {
	return _syscall_generic(func_id, arg1, 0, 0, 0, 0);
}

uint32_t _syscall_generic_0(uint32_t func_id) {
	return _syscall_generic(func_id, 0, 0, 0, 0, 0);
}
