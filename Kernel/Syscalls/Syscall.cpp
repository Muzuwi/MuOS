#include <Core/Log/Logger.hpp>
#include <Process/Process.hpp>
#include <Syscalls/Syscall.hpp>

CREATE_LOGGER("syscall", core::log::LogLevel::Debug);

template<typename... Args>
static uint64_t call(void* ptr, Args... args) {
	return reinterpret_cast<uint64_t (*)(Args...)>(ptr)(args...);
}

struct _SyscallHandler {
	unsigned _argc;
	void* _ptr;
	bool _ret;
};

_SyscallHandler s_syscall_functions[256];

void Syscall::init() {
#define DEFINE_SYSCALL(idx, ptr, argc, ret)                   \
	s_syscall_functions[idx]._ptr = (void*)ptr;               \
	s_syscall_functions[idx]._argc = argc;                    \
	s_syscall_functions[idx]._ret = ret;                      \
	static_assert(idx < 256, "Syscall function ID too big!"); \
	static_assert(ret == true || ret == false, "Syscall _ret value must be true or false");

	SYSCALL_ENUMERATE

#undef DEFINE_SYSCALL
}

[[maybe_unused]] void Syscall::syscall_handle(PtraceRegs* regs) {
	auto id = (uint8_t)regs->rax;
	auto& handler = s_syscall_functions[id];
	if(!handler._ptr) {
		log.error("Invalid syscall to nonexistent function ID={}", id);
		return;
	}

	log.debug("Thread({}): syscall={}", Thread::current()->tid(), id);

	auto argc = handler._argc;
	auto ptr = handler._ptr;
	bool has_retval = handler._ret;

	//  Beautiful call() pyramid
	switch(argc) {
		case 0: {
			regs->rax = call(ptr);
			break;
		}
		case 1: {
			regs->rax = call(ptr, regs->rdi);
			break;
		}
		case 2: {
			regs->rax = call(ptr, regs->rdi, regs->rsi);
			break;
		}
		case 3: {
			regs->rax = call(ptr, regs->rdi, regs->rsi, regs->rdx);
			break;
		}
		case 4: {
			regs->rax = call(ptr, regs->rdi, regs->rsi, regs->rdx, regs->r8);
			break;
		}
		case 5: {
			regs->rax = call(ptr, regs->rdi, regs->rsi, regs->rdx, regs->r8, regs->r9);
			break;
		}
		default: {
			log.error("Corrupted argc={} for syscall_id={}", argc, id);
			return;
		}
	}

	//  Zero out retval if the syscall is not supposed to return a value, to prevent leaking kernel data
	if(!has_retval)
		regs->rax = 0;
}
