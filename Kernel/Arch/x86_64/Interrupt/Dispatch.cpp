#include <Arch/x86_64/PtraceRegs.hpp>
#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/IRQ/IRQ.hpp>
#include <Core/MP/MP.hpp>
#include <Core/Task/SchedulerPlatform.hpp>

extern "C" void _kernel_irq_dispatch(uint8_t irq, PtraceRegs* interrupt_trap_frame) {
	core::irq::dispatch(core::irq::IrqId { irq }, interrupt_trap_frame);
	core::sched::on_interrupt_exit();
}
