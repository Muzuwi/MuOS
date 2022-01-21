#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Kernel/Scheduler/RunQueue.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kassert.hpp>
#include <Kernel/SMP/SMP.hpp>
#include <Arch/x86_64/PortIO.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <Kernel/APIC/APIC.hpp>
#include <Kernel/SMP/APBootstrap.hpp>
#include <string.h>
#include "ksleep.hpp"
#include <Kernel/Structs/StaticRing.hpp>

[[noreturn]] void _kernel_idle_task() {
	while(true)
			asm volatile("hlt");
}

[[noreturn]] void _kernel_test_task_2() {
	while(true) {
		kdebugf("test thread2, pid=%i, tid=%i\n", Thread::current()->parent()->pid(), Thread::current()->tid());
		Thread::current()->msleep(2000);
	}
}

static StaticRing<uint8, 1024> s_keyboard_buffer {};

DEFINE_MICROTASK(_kbd_microtask) {
	while(Ports::in(0x64) & 1u) {
		auto ch = Ports::in(0x60);
		s_keyboard_buffer.try_push(ch);
	}
}

[[noreturn]] void _kbd() {
	while(true) {
		while(!s_keyboard_buffer.empty()) {
			auto byte = s_keyboard_buffer.try_pop();
			kdebugf("kbd(%i): byte %x\n", Thread::current()->tid(), byte.unwrap());
		}
		Thread::current()->set_state(TaskState::Blocking);
		SMP::ctb().scheduler().schedule();
	}
}

extern uint8 ap_bootstrap_start;
extern uint8 ap_bootstrap_end;

[[noreturn]] void Scheduler::bootstrap_ap_task() {
	kdebugf("[Startup(%i)]: Booting up APs\n", Thread::current()->tid());
	auto* thread = SMP::ctb().current_thread();
	auto const& process = thread->parent();

	auto maybe_code_page = PMM::instance().allocate_lowmem();
	if(!maybe_code_page.has_value()) {
		kdebugf("[Startup(%i)]: Lowmem alloc for code failed\n", thread->tid());
		Thread::current()->set_state(TaskState::Blocking);
		SMP::ctb().scheduler().schedule();
		ASSERT_NOT_REACHED();
	}
	auto maybe_data_page = PMM::instance().allocate_lowmem();
	if(!maybe_data_page.has_value()) {
		kdebugf("[Startup(%i)]: Lowmem alloc for data failed\n", thread->tid());
		Thread::current()->set_state(TaskState::Blocking);
		SMP::ctb().scheduler().schedule();
		ASSERT_NOT_REACHED();
	}

	auto code_page = maybe_code_page.unwrap();
	auto data_page = maybe_data_page.unwrap();
	process->vmm().addrmap(code_page.get(), code_page,
	                       static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL));
	process->vmm().addrmap(data_page.get(), data_page,
	                       static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL));
	kdebugf("[Startup(%i)]: Code at %x, data at %x\n", thread->tid(), code_page.get(), data_page.get());

	const uint8 ipi_vector = (uintptr_t)code_page.get() / 0x1000;
	kassert(ipi_vector < 0xA0 || ipi_vector > 0xBF);


	for(auto& ap_id : APIC::ap_list()) {
		if(ap_id == APIC::ap_bootstrap_id()) {
			continue;
		}
		kdebugf("[Startup(%i)]: Booting up AP %i\n", thread->tid(), ap_id);

		memcpy(code_page.get_mapped(),
		       static_cast<void const*>(&ap_bootstrap_start),
		       &ap_bootstrap_end - &ap_bootstrap_start);
		//  Pass the data page address
		*(code_page.as<uint16>() + 1) = (uintptr_t)data_page.get();

		auto idle_task = SMP::ctb().scheduler().create_idle_task();
		APBoostrap bootstrap_struct {};
		bootstrap_struct.real_gdtr_offset = (uintptr_t)data_page.get() + offsetof(APBoostrap, real_mode_gdt);
		bootstrap_struct.compat_gdtr_offset = (uintptr_t)data_page.get() + offsetof(APBoostrap, compat_mode_gdt);
		bootstrap_struct.long_gdtr_offset = (uintptr_t)data_page.get() + offsetof(APBoostrap, long_mode_gdt);
		bootstrap_struct.cr3 = (uintptr_t)idle_task->parent()->vmm().pml4().get();

		//  Copy over the boot struct
		memcpy(data_page.get_mapped(),
		       &bootstrap_struct,
		       sizeof(APBoostrap));

		//  Try booting up the AP
		const uint32 id_msg = (uint32)ap_id << 24u;
		const uint32 lo_init = 0xC500;          //  Init IPI
		const uint32 lo_deassert_init = 0x8500; //  De-assert init IPI

		thread->preempt_disable();
		APIC::lapic_write(LAPICReg::ESR, 0x0);
		APIC::lapic_write(LAPICReg::ICRHi, id_msg);
		APIC::lapic_write(LAPICReg::ICRLow, lo_init);
		while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))  //  Wait for delivery
			;
		APIC::lapic_write(LAPICReg::ICRLow, lo_deassert_init);
		while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))  //  Wait for delivery
			;

		ksleep(10);

		for(unsigned i = 0; i < 2; ++i) {
			APIC::lapic_write(LAPICReg::ESR, 0x0);
			APIC::lapic_write(LAPICReg::ICRHi, id_msg);
			APIC::lapic_write(LAPICReg::ICRLow, 0x600 | ipi_vector);
			ksleep(1);
			while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))  //  Wait for delivery
				;
		}
		thread->preempt_enable();

		while(*data_page.as<uint64>() == 0);
		kdebugf("[Startup(%i)]: AP %i started - waiting for long mode\n", thread->tid(), ap_id);
		while(*data_page.as<uint64>() != 2);
		kdebugf("[Startup(%i)]: AP %i initialized\n", thread->tid(), ap_id);
//		break;
	}

	kdebugf("[Startup(%i)]: Initialization done\n", thread->tid());

	process->vmm().addrunmap(code_page.get());
	process->vmm().addrunmap(data_page.get());

	PMM::instance().free_lowmem(code_page);
	PMM::instance().free_lowmem(data_page);

	Thread::current()->set_state(TaskState::Blocking);
	SMP::ctb().scheduler().schedule();
	ASSERT_NOT_REACHED();
}


void Scheduler::tick() {
	auto* thread = SMP::ctb().current_thread();
	if(!thread) {
		return;
	}

	if(thread == m_ap_idle) {
		//  Force reschedule when at least one thread becomes runnable
		if(m_rq.find_runnable() != nullptr) {
			thread->reschedule();
		} else {
			thread->clear_reschedule();
		}
		return;
	}

	//  Cannot preempt, process is holding locks
	if(thread->preempt_count() > 0) {
		return;
	}

	//  Spend one process quantum
	if(thread->sched_ctx().quants_left) {
		thread->sched_ctx().quants_left--;
	} else {
		//  Reschedule currently running thread - ran out of quants
		thread->reschedule();
		thread->sched_ctx().quants_left = pri_to_quants(120 + thread->priority());
	}
}


/*
 *  Called when a task voluntarily relinquishes its' CPU time
 */
void Scheduler::schedule() {
	auto* thread = SMP::ctb().current_thread();
	thread->preempt_disable();

	//  Find next runnable task
	auto* next_thread = m_rq.find_runnable();
	if(!next_thread) {
		next_thread = m_ap_idle;
	}

	CPU::switch_to(thread, next_thread);
	thread->preempt_enable();
}


void Scheduler::interrupt_return_common() {
	auto* current = SMP::ctb().current_thread();
	if(!current) {
		return;
	}
	if(!current->needs_reschedule()) {
		return;
	}

	current->clear_reschedule();
	schedule();
}


void Scheduler::wake_up(Thread* thread) {
	if(!thread) { return; }

	assert(thread->state() != TaskState::Running);
	thread->set_state(TaskState::Ready);

	auto* current = SMP::ctb().current_thread();
	if(thread->priority() < current->priority()) {
		current->reschedule();
	}
}


Thread* Scheduler::create_idle_task() {
	auto thread = Thread::create_in_process(Process::kerneld(), _kernel_idle_task);
	return thread.get();
}


/*
 *  Creates the idle task for the current AP and enters it, kickstarting the scheduler on the current AP
 */
void Scheduler::bootstrap() {
	CPU::irq_disable();
	kdebugf("[Scheduler] Bootstrapping AP %i\n", SMP::ctb().current_ap());

	m_ap_idle = create_idle_task();

	{
		auto new_process = Process::init();
		new_process->vmm().m_pml4 = new_process->vmm().clone_pml4(Process::kerneld()->vmm().m_pml4).unwrap();

		auto new_thread = Thread::create_in_process(new_process, [] {
			auto* current = Thread::current();

			//  Create a userland stack for the thread
			void* user_stack = current->parent()->vmm().allocate_user_stack(VMM::user_stack_size());

			const uint8 bytes[] = { 0x48, 0xB8, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00, 0x00, 0x00, 0x50, 0x48, 0xB8, 0x48,
			                        0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x77, 0x50, 0x48, 0x89, 0xE7, 0x48, 0xC7, 0xC0,
			                        0xFF, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC0, 0x64, 0x00, 0x00, 0x00, 0x48,
			                        0xC7, 0xC7, 0x00, 0x40, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC3, 0xFF, 0xFF, 0xFF,
			                        0xFF, 0x48, 0xC7, 0xC1, 0x00, 0x40, 0x00, 0x00, 0x48, 0x89, 0x18, 0x48, 0x83, 0xC0,
			                        0x08, 0x48, 0x83, 0xE9, 0x08, 0x48, 0x83, 0xF9, 0x00, 0x75, 0xEF, 0x48, 0xC7, 0xC3,
			                        0x0A, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0xE8, 0x03, 0x00, 0x00, 0x48, 0xC7, 0xC0,
			                        0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xFF, 0xCB, 0x48, 0x83, 0xFB, 0x00, 0x75,
			                        0xE7, 0x48, 0xC7, 0xC0, 0x64, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x00, 0x10, 0x00,
			                        0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0xC7, 0xC1, 0x00,
			                        0x10, 0x00, 0x00, 0x48, 0x89, 0x18, 0x48, 0x83, 0xC0, 0x08, 0x48, 0x83, 0xE9, 0x08,
			                        0x48, 0x83, 0xF9, 0x00, 0x75, 0xEF, 0x48, 0xC7, 0xC7, 0xE8, 0x03, 0x00, 0x00, 0x48,
			                        0xC7, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xEB, 0xEE };
			auto* shellcode_location = (uint8*)0x100000;

			kdebugf("Thread(%i): mapping shellcode\n", current->tid());
			auto mapping = VMapping::create((void*)shellcode_location, 0x1000, VM_READ | VM_WRITE | VM_EXEC,
			                                MAP_SHARED);
			kassert(current->parent()->vmm().insert_vmapping(gen::move(mapping)));

			kdebugf("Thread(%i): copying shellcode\n", current->tid());
			for(auto& b : bytes) {
				*shellcode_location = b;
				shellcode_location++;
			}

			kdebugf("Thread(%i): jumping to user\n", current->tid());
			PtraceRegs regs = PtraceRegs::user_default();
			regs.rip = 0x100000;
			regs.rsp = (uint64)user_stack;
			CPU::jump_to_user(&regs);
		});
		new_thread->sched_ctx().priority = 10;
		add_thread_to_rq(new_thread.get());
	}

	{
		auto new_process = Process::kerneld();
		auto new_thread = Thread::create_in_process(new_process, _kernel_test_task_2);
		add_thread_to_rq(new_thread.get());
	}

	{
		auto keyboard_thread = Thread::create_in_process(Process::kerneld(), _kbd);
		add_thread_to_rq(keyboard_thread.get());
		IRQDispatcher::register_microtask(1, _kbd_microtask);
		IRQDispatcher::register_threadirq(1, keyboard_thread.get());
	}

	{
		auto ap_start_thread = Thread::create_in_process(Process::kerneld(), Scheduler::bootstrap_ap_task);
		add_thread_to_rq(ap_start_thread.get());
	}

	uint8 _dummy_val[sizeof(Thread)] = {};
	//  Huge hack - use dummy buffer on the stack when switching for the first time,
	//  as we don't care about saving garbage data
	CPU::switch_to(reinterpret_cast<Thread*>(&_dummy_val), m_ap_idle);
}

unsigned Scheduler::pri_to_quants(uint8_t priority) {
	kassert(priority <= 140);

	if(priority < 120) {
		return (140 - priority) * 20;
	} else {
		return (140 - priority) * 5;
	}
}

void Scheduler::add_thread_to_rq(Thread* thread) {

	//  FIXME/SMP: Locking
	m_rq.add(thread);

}

