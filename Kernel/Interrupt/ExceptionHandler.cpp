#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Interrupt/Exception.hpp>

static void* s_exception_handlers[32] {
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

extern "C" void _kernel_exception_entrypoint(size_t vector) {
	kerrorf("[Exception] vector=%x\n", vector);
	if(!s_exception_handlers[vector])
		kpanic();
	auto handler = reinterpret_cast<void(*)()>(s_exception_handlers[vector]);
	handler();
}

extern "C" void _kernel_exception_errorcode_entrypoint(size_t vector, size_t error_code) {
	kerrorf("[Exception] vector=%x, error_code=%x\n", vector, error_code);
	if(!s_exception_handlers[vector])
		kpanic();
	auto handler = reinterpret_cast<void(*)(size_t)>(s_exception_handlers[vector]);
	handler(error_code);
}