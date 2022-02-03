#include <Arch/x86_64/V86.hpp>
#include <Memory/PMM.hpp>
#include <Memory/Ptr.hpp>
#include <Memory/VMM.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <SMP/SMP.hpp>

extern "C" void vm86_irq(PhysAddr, PhysAddr, uint64 irq, V86Regs&);

void V86::run_irq(uint8 irq, V86Regs& regs) {
	auto* thread = SMP::ctb().current_thread();
	auto const& process = thread->parent();

	auto maybe_code_page = PMM::instance().allocate_lowmem();
	auto maybe_stack_page = PMM::instance().allocate_lowmem();
	kassert(maybe_code_page.has_value());
	kassert(maybe_stack_page.has_value());

	auto code_page = maybe_code_page.unwrap();
	auto stack_page = maybe_stack_page.unwrap();

	process->vmm().addrmap(code_page.get(),
	                       code_page,
	                       static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL ));
	process->vmm().addrmap(stack_page.get(),
	                       stack_page,
	                       static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_KERNEL ));

	vm86_irq(PhysAddr { code_page.get() }, PhysAddr { stack_page.get() }, irq, regs);

	process->vmm().addrunmap(code_page.get());
	process->vmm().addrunmap(stack_page.get());
	PMM::instance().free_lowmem(code_page);
	PMM::instance().free_lowmem(stack_page);
}
