#include <Arch/x86_64/PtraceRegs.hpp>
#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/IRQ/IRQ.hpp>
#include <Core/MP/MP.hpp>
#include <Scheduler/Scheduler.hpp>

extern "C" void _kernel_irq_dispatch(uint8_t irq, PtraceRegs* interrupt_trap_frame) {
	core::irq::dispatch(core::irq::IrqId { irq }, interrupt_trap_frame);
	this_cpu()->scheduler->interrupt_return_common();
}
