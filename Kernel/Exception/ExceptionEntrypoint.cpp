#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Exception/Exception.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/SMP/SMP.hpp>

static Exception::HandlerFunction s_exception_handlers[32] {
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, Exception::handle_page_fault, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

extern "C" void _kernel_exception_entrypoint(size_t vector, PtraceRegs* interrupt_stack_frame) {
	auto handler = reinterpret_cast<Exception::HandlerFunction>(s_exception_handlers[vector]);
	if(!s_exception_handlers[vector]) {
		handler = Exception::handle_uncaught;
	}

	auto response = handler(interrupt_stack_frame, vector);
	if(response == Exception::Response::KernelPanic) {
		kpanic();
	} else if(response == Exception::Response::TerminateThread) {
		const auto thread = SMP::ctb().current_thread();
		thread->set_state(TaskState::Leaving);
		thread->reschedule();
		SMP::ctb().scheduler().schedule();
	}
}