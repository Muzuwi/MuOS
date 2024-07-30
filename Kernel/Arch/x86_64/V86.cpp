#include <Arch/x86_64/V86.hpp>
#include <Core/MP/MP.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Memory/Ptr.hpp>
#include <Memory/VMM.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>

//  Defines the addresses of the V86 code and stack pages.
//  These MUST be accessible from 16-bit real mode!
//  The addresses are reserved at boot time (and thus won't be
//  randomly given out by GFP), when the kernel reserves
//  the lower conventional memory on x86.
#define CONFIG_ARCH_X86_64_V86_CODE_PAGE  ((void*)0xa000)
#define CONFIG_ARCH_X86_64_V86_STACK_PAGE ((void*)0xb000)

//  Because of course locking is our topmost concern when we're destroying
//  literally all microarchitectural state by going Long->Protected->Real,
//  executing a random BIOS interrupt, and going Real->Protected->Long again
//  and expecting everything to work exactly how we left it.
static gen::Spinlock s_lock {};

extern "C" void vm86_irq(PhysAddr, PhysAddr, uint64 irq, V86Regs&);

void V86::run_irq(uint8 irq, V86Regs& regs) {
	gen::LockGuard lg { s_lock };

	auto* thread = this_cpu()->current_thread();
	auto const& process = thread->parent();

	auto code_page = PhysAddr { CONFIG_ARCH_X86_64_V86_CODE_PAGE };
	auto stack_page = PhysAddr { CONFIG_ARCH_X86_64_V86_STACK_PAGE };

	process->vmm().addrmap(code_page.get(), code_page,
	                       static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL));
	process->vmm().addrmap(stack_page.get(), stack_page, static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_KERNEL));

	vm86_irq(PhysAddr { code_page.get() }, PhysAddr { stack_page.get() }, irq, regs);

	process->vmm().addrunmap(code_page.get());
	process->vmm().addrunmap(stack_page.get());
}
