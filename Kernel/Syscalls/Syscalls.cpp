#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <Kernel/Syscalls/SyscallList.hpp>
#include <LibGeneric/Vector.hpp>

struct _SyscallDefinition {
	uint8_t _argcount;
	void* _entry;
};

static _SyscallDefinition syscall_lookup[256] {};


void Syscall::init() {
#define _ENUMERATE_SYSCALL(name, argc) \
    syscall_lookup[__SYS_##name]._entry = reinterpret_cast<void*>(&Process::name); \
    syscall_lookup[__SYS_##name]._argcount = argc;

	ENUM_SYSCALLS

#undef _ENUMERATE_SYSCALL
}

uint32_t Syscall::dispatch(uint32_t function_id, const _SyscallParamPack& args) {
	ASSERT_IRQ_DISABLED();

	kdebugf("[Syscall] New syscall %i\n", function_id);

	if(function_id > 256) {
		kerrorf("[Syscall] Invalid syscall ID called (%i)\n", function_id);
		return -1;
	}

	auto func = syscall_lookup[function_id]._entry;
	auto argc = syscall_lookup[function_id]._argcount;

	if(!func) {
		kerrorf("[Syscall] Null sycall called (%i) [%x]\n", function_id, func);
		return -1;
	}

	//  exit() should not return, thus handle it independently from other syscalls
	if(function_id == __SYS_exit) {
		Process::exit(args.arg1);
	}

	if(argc == 0)
		return reinterpret_cast<_Syscall_Arg0>(func)();
	else if(argc == 1)
		return reinterpret_cast<_Syscall_Arg1>(func)(args.arg1);
	else if(argc == 2)
		return reinterpret_cast<_Syscall_Arg2>(func)(args.arg1, args.arg2);
	else if(argc == 3)
		return reinterpret_cast<_Syscall_Arg3>(func)(args.arg1, args.arg2, args.arg3);
	else if(argc == 4)
		return reinterpret_cast<_Syscall_Arg4>(func)(args.arg1, args.arg2, args.arg3, args.arg4);
	else if(argc == 5)
		return reinterpret_cast<_Syscall_Arg5>(func)(args.arg1, args.arg2, args.arg3, args.arg4, args.arg5);

	kerrorf("[Syscall] Invalid argument count in lookup table (%i)!\n", argc);
	return -1;
}