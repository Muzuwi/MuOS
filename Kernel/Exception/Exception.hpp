#pragma once
#include <SystemTypes.hpp>
#include <Arch/x86_64/PtraceRegs.hpp>

enum class ExceptionType {
	DivZero         = 0,
	Debug           = 1,
	NMI             = 2,
	Breakpoint      = 3,
	Overflow        = 4,
	__BoundOvflw    = 5,
	InvalidOpcode   = 6,
	FPUMissing      = 7,
	DoubleFault     = 8,
	__Reserved9     = 9,
	InvalidTSS      = 10,
	SegNotPresent   = 11,
	SSFault         = 12,
	GPFault         = 13,
	PageFault       = 14,
	__Reserved15    = 15,
	x87FPU          = 16,
	AlignCheck      = 17,
	MachineCheck    = 18,
	SIMDFPU         = 19,
	VirtExcept      = 20,
	__Reserved21    = 21,
	__Reserved22    = 22,
	__Reserved23    = 23,
	__Reserved24    = 24,
	__Reserved25    = 25,
	__Reserved26    = 26,
	__Reserved27    = 27,
	__Reserved28    = 28,
	__Reserved29    = 29,
	__SecurityEx    = 30,
	__Reserved31    = 31
};

extern "C" void _kernel_exception_entrypoint(size_t vector, PtraceRegs* interrupt_stack_frame);

namespace Exception {
	enum class Response {
		TerminateThread,
		KernelPanic,
		Resume
	};
	typedef Response(*HandlerFunction)(PtraceRegs* frame, uint8 vector);

	Response handle_uncaught(PtraceRegs*, uint8 vector);
	Response handle_page_fault(PtraceRegs*, uint8);
}
