#pragma once
#include <sys/types.h>
#include <asm/unistd.h>
#include <Kernel/SystemTypes.hpp>

typedef uint32_t _sysret_t;
typedef uint32_t _sysarg_t;
typedef _sysret_t (*_Syscall_Arg0)();
typedef _sysret_t (*_Syscall_Arg1)(_sysarg_t);
typedef _sysret_t (*_Syscall_Arg2)(_sysarg_t, _sysarg_t);
typedef _sysret_t (*_Syscall_Arg3)(_sysarg_t, _sysarg_t, _sysarg_t);
typedef _sysret_t (*_Syscall_Arg4)(_sysarg_t, _sysarg_t, _sysarg_t, _sysarg_t);
typedef _sysret_t (*_Syscall_Arg5)(_sysarg_t, _sysarg_t, _sysarg_t, _sysarg_t, _sysarg_t);

#define ENUM_SYSCALLS \
	_ENUMERATE_SYSCALL(exit, 1)       \
	_ENUMERATE_SYSCALL(write, 3)      \
	_ENUMERATE_SYSCALL(sleep, 1)      \
	_ENUMERATE_SYSCALL(getpid, 0)     \
	_ENUMERATE_SYSCALL(getuid, 0)

enum class SyscallNumber {
#define _ENUMERATE_SYSCALL(name,argc) name = __SYS_##name,
	ENUM_SYSCALLS
#undef _ENUMERATE_SYSCALL
};

struct _SyscallParamPack {
	_sysarg_t arg1;
	_sysarg_t arg2;
	_sysarg_t arg3;
	_sysarg_t arg4;
	_sysarg_t arg5;
};

namespace Syscall {
	void init();
	uint32_t dispatch(uint32_t, const _SyscallParamPack&);
}


