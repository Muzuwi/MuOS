#pragma once
#include <sys/types.h>
#include <Kernel/SystemTypes.hpp>

enum class SyscallNumber {
	Exit = 1,
	Write = 2,
	Sleep = 3
};

struct _SyscallParamPack {
	uint32_t arg1;
	uint32_t arg2;
	uint32_t arg3;
	uint32_t arg4;
	uint32_t arg5;
	_SyscallParamPack(uint32_t _arg1, uint32_t _arg2, uint32_t _arg3, uint32_t _arg4, uint32_t _arg5)
	: arg1(_arg1), arg2(_arg2), arg3(_arg3), arg4(_arg4), arg5(_arg5) {}

	template<const unsigned count, class T>
	T get() const {
		static_assert(count <= 5, "Invalid parameter pack number");

		if constexpr(count == 1)
			return reinterpret_cast<T>(arg1);
		else if constexpr(count == 2)
			return reinterpret_cast<T>(arg2);
		else if constexpr(count == 3)
			return reinterpret_cast<T>(arg3);
		else if constexpr(count == 4)
			return reinterpret_cast<T>(arg4);
		else if constexpr(count == 5)
			return reinterpret_cast<T>(arg5);
	}
};



class Syscall {
	template<class T>
	static bool verify_read(T*);

	template<class T>
	static bool verify_write(T*);
public:
	static uint32_t dispatch(uint32_t, const _SyscallParamPack&);
	[[noreturn]] static void exit(int retval);

	static void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset);
	static size_t write(int fildes, const void* buf, size_t nbyte);
	static unsigned sleep(unsigned seconds);

};


