#include <Kernel/Interrupt/Exception.hpp>
#include <Arch/i386/Registers.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <Kernel/Debug/kpanic.hpp>

static uint32_t cr2() {
	uint32_t tmp;
	asm volatile(
	"mov %0, cr2\n"
	:"=a"(tmp)
	);
	return tmp;

}

static void dump_stack_trace(uint32_t* base_frame) {
	kerrorf("Stack trace:\n");
	auto* frame = (uint32_t*)base_frame;
	while(frame) {
		//  FIXME:  Bandaid
		bool valid = Process::current() ? (Process::verify_read<uint32_t>(frame+1) && Process::verify_read<uint32_t>(frame))
		                                : (frame >= &_ukernel_virtual_offset);

		if(valid){
			kerrorf("    at %x <next frame at %x>\n", *(frame + 1), frame);
			frame = (uint32_t*)*frame;
		} else {
			kerrorf("    <next frame invalid>\n");
			break;
		}
	}
}

static void dump_registers(x86::IRQFrame regs) {
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.handler_esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
}


static ExceptionResponse _kernel_unhandled_exception(size_t vector, x86::IRQFrame frame) {
	bool is_kernel_exception = (frame.CS == 0x8);

	kerrorf("%s(%i): Unhandled exception no. %x at %x\n",
	        is_kernel_exception ? "Kernel" : "Process",
	        Process::current() ? Process::current()->pid() : 0, vector, frame.eip);

	dump_registers(frame);
	dump_stack_trace((uint32_t*)frame.ebp);

	if(is_kernel_exception) {
		return ExceptionResponse::KernelPanic;
	} else {
		return ExceptionResponse::TerminateProcess;
	}
}

static ExceptionResponse page_fault_handler(x86::IRQFrame frame) {
	bool is_kernel_exception = !(frame.error_code & 4);

	kerrorf("%s(%i): Page Fault exception at %x\n",
	        is_kernel_exception ? "Kernel" : "Process",
	        Process::current() ? Process::current()->pid() : 0,
	        frame.eip);

	dump_registers(frame);
	dump_stack_trace((uint32_t*)frame.ebp);

	kerrorf("Error code: %x [%s - caused by a %s, CPL %i]\n", frame.error_code,
	        frame.error_code & 1 ? "Page-Protection violation" : "Non-present page",
	        frame.error_code & 2 ? "write" : "read",
	        frame.error_code & 4 ? 3 : 0);

	if(cr2() == 0) {
		kerrorf("Notice: Possible null pointer dereference - tried accessing address 0\n");
	} else if(cr2() == 0x99) {
		kerrorf("Notice: Possible uninitialized memory - tried accessing address 0x99 (KM sanitizer byte)\n");
	}

	return is_kernel_exception ? ExceptionResponse::KernelPanic : ExceptionResponse::TerminateProcess;
}


static Exception::HandlerFunction s_exception_handlers[32] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,&page_fault_handler,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static Exception::HandlerFunction get_handler_for(size_t vector) {
	if(vector >= 32)
		return nullptr;
	else
		return s_exception_handlers[vector];
}

bool Exception::register_except_handler(ExceptionType type, Exception::HandlerFunction handler) {
	if(!handler)
		return false;
	if(static_cast<size_t>(type) >= 32)
		return false;

	s_exception_handlers[static_cast<size_t>(type)] = handler;
}


static x86::IRQFrame _to_handler_frame(TrapFrame frame) {
	return {frame.edi,frame.esi,frame.ebp,frame.handler_esp,frame.ebx,frame.edx,frame.ecx,frame.eax,0,frame.eip,frame.CS,frame.EFLAGS};
}

static x86::IRQFrame _to_handler_frame(ErrorCodeTrapFrame frame) {
	return {frame.edi,frame.esi,frame.ebp,frame.handler_esp,frame.ebx,frame.edx,frame.ecx,frame.eax,frame.error_code,frame.eip,frame.CS,frame.EFLAGS};
}

extern "C" void _kernel_exception_entrypoint(size_t vector, TrapFrame frame) {
	auto handler = get_handler_for(vector);

	auto res = (handler ? handler(_to_handler_frame(frame))
	                    : _kernel_unhandled_exception(vector, _to_handler_frame(frame)));

	if(res == ExceptionResponse::KernelPanic) {
		kpanic();
	} else if(res == ExceptionResponse::TerminateProcess) {
		Process::kill(Process::current()->pid());
		Scheduler::switch_task();
	}
}

extern "C" void _kernel_exception_errorcode_entrypoint(size_t vector, ErrorCodeTrapFrame frame) {
	auto handler = get_handler_for(vector);

	auto res = (handler ? handler(_to_handler_frame(frame))
						: _kernel_unhandled_exception(vector, _to_handler_frame(frame)));

	if(res == ExceptionResponse::KernelPanic) {
		kpanic();
	} else if(res == ExceptionResponse::TerminateProcess) {
		Process::kill(Process::current()->pid());
		Scheduler::switch_task();
	}
}