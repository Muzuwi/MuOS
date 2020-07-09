#include <Kernel/Process/Scheduler.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Interrupt/IRQSubscriber.hpp>
#include <include/Arch/i386/CPU.hpp>
#include <include/Kernel/Debug/kassert.hpp>
#include <LibGeneric/Algorithm.hpp>
#include <include/Arch/i386/GDT.hpp>

//#define SCHEDULE_LOG
//#define SCHEDULE_LOG_DUMP_REG_SWITCH

static bool ready = false;

static gen::List<Process*> s_processes {};


[[noreturn]] void _kernel_idle_task() {
	while(true)
		asm volatile("hlt");
}

/*
 *  Initializes the kernel idle task
 */
void Scheduler::initialize() {
	IRQDisabler disabler;
	kdebugf("[Scheduler] Init kernel idle task\n");

	Process::m_kernel_idle = new Process(0, Ring::CPL0, {(void*)_kernel_idle_task, 0, ExecutableType::Flat});
	s_processes.clear();

	new IRQSubscriber(0, []{
		static unsigned ticksUntilSwitch = 20;
		if(ticksUntilSwitch-- == 0) {
			ticksUntilSwitch = 20;
			Scheduler::switch_task();
		}
	}, SubscriberPriority::MayTaskSwitch);
}

/*
 *  Leaves the kernel and starts the scheduling loop
 */
void Scheduler::enter_scheduler_loop() {
	asm volatile("cli\n");

	ready = true;
	Process::m_current = Process::m_kernel_idle;
	Process::m_current->finalize();
}

void Scheduler::notify_new_process(Process* v) {
	ASSERT_IRQ_DISABLED();
	s_processes.push_back(v);
}

//  FIXME: Change the entire scheduler
Process* Scheduler::pick_next() {
	ASSERT_IRQ_DISABLED();
	if(s_processes.size() == 0) {   //  FIXME:  Bug in gen::list
		return Process::m_kernel_idle;
	}
	else if(Process::m_current == Process::m_kernel_idle) {
		if(s_processes.front()->m_state == ProcessState::Sleeping)
			return Process::m_kernel_idle;
		else
			return s_processes.front();
	}
	else {
		auto pr = gen::find_if(s_processes, [](Process* proc) -> bool {
			return proc->m_pid == Process::m_current->pid();
		});
		while(true) {
			pr = ++pr;

			if((*pr)->m_state == ProcessState::Sleeping) continue;

			if(pr != s_processes.end()) {
				return *pr;
			}
			else {
				if(s_processes.front()->m_state == ProcessState::Sleeping)
					return Process::m_kernel_idle;
				else
					return s_processes.front();
			}
		}
	}
}

SchedulerAction Scheduler::handle_process_pick(Process* next_process) {
	ASSERT_IRQ_DISABLED();
	switch(next_process->m_state) {
		case ProcessState::New:
		case ProcessState::Ready:
			return SchedulerAction::Use;

		case ProcessState::Leaving: {
			kdebugf("[Scheduler] Cleaning up process PID %i \n", next_process->m_pid);

			auto it = gen::find_if(s_processes, [&](Process* proc) -> bool {
				return proc->m_pid == next_process->m_pid;
			});
			if(it != s_processes.end()) {
				s_processes.erase(it);
			}

			it = gen::find_if(Process::m_all_processes, [&](Process* proc) -> bool {
				return proc->m_pid == next_process->m_pid;
			});
			if(it != Process::m_all_processes.end()) {
				Process::m_all_processes.erase(it);
			}

			delete next_process;

			return SchedulerAction::PickAgain;
		}

		case ProcessState::Blocking:
			//  TODO: Check if block is done, if so we can use this process, otherwise pick another
			kpanic();
		case ProcessState::Running:
			kpanic();
		case ProcessState::Sleeping:
			kpanic();
		default:
			kpanic();
	}
}

void Scheduler::yield_with_irq_frame(uint32_t esp) {
	if (!ready) return;
	ASSERT_IRQ_DISABLED();

//	kdebugf("Process(%i): yield - irq frame %x\n", Process::m_current->m_pid, esp);

	Process::m_current->m_current_irq_trap_frame = reinterpret_cast<TrapFrame*>(esp);
	Process::m_current->fpu_state().store();

	if (Process::m_current->m_state == ProcessState::Running)
		Process::m_current->m_state = ProcessState::Ready;

	auto* next_process = Scheduler::pick_next();
	while (Scheduler::handle_process_pick(next_process) != SchedulerAction::Use)
		next_process = Scheduler::pick_next();

	Process::m_current = next_process;
	Process::m_current->set_state(ProcessState::Running);
	GDT::set_TSS_stack((void*)(0xdd004000));

	//  Bootstrap task
	if (!Process::m_current->is_finalized()) {
		Process::m_current->finalize();
	}
	//  Resume paused task
	else {
		Process::m_current->fpu_state().restore();
		CPU::load_segment_registers_for(Process::m_current->ring());
		CPU::jump_to_irq_frame(Process::m_current->m_current_irq_trap_frame, Process::m_current->directory());
	}
}
