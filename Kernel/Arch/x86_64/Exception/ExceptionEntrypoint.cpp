#include <Core/MP/MP.hpp>
#include <Debug/kpanic.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include "Exception.hpp"

static Exception::HandlerFunction s_exception_handlers[32] {
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, Exception::handle_page_fault,
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr
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
		const auto thread = this_cpu()->current_thread();
		thread->set_state(TaskState::Leaving);
		thread->reschedule();
		this_cpu()->scheduler->block();
	}
}