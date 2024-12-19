#include <Core/Assert/Assert.hpp>
#include <Core/MP/MP.hpp>
#include <Core/Task/Scheduler.hpp>
#include <Core/Task/Task.hpp>
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
		ENSURE_NOT_REACHED();
	} else if(response == Exception::Response::TerminateThread) {
		core::sched::exit();
	}
}