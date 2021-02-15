#include <Kernel/Exception/Exception.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>

static Exception::HandlerFunction s_exception_handlers[32] {
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

extern "C" void _kernel_exception_entrypoint(size_t vector, void* interrupt_stack_frame) {
	kerrorf("[Exception] vector=%x\n", vector);

	if(!s_exception_handlers[vector]) {
		kerrorf("[Exception] Unhandled exception vector=%x\n", vector);
		kpanic();
	}

	auto handler = reinterpret_cast<Exception::HandlerFunction>(s_exception_handlers[vector]);
	auto response = handler(interrupt_stack_frame);
	if(response == Exception::Response::KernelPanic) {
		kpanic();
	} else if(response == Exception::Response::TerminateProcess) {
		//  Terminate process
	}
}