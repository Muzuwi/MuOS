#include <Kernel/Process/Scheduler.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Interrupt/IRQSubscriber.hpp>

//#define SCHEDULE_LOG
//#define SCHEDULE_LOG_DUMP_REG_SWITCH

static bool ready = false;

static gen::List<Process*> s_processes;

auto find(gen::BidirectionalIterator<gen::List<Process*>> begin,
		gen::BidirectionalIterator<gen::List<Process*>> end,
		pid_t pid ) -> gen::BidirectionalIterator<gen::List<Process*>> {
	auto it = begin;
	while(it != end) {
		if((*it)->pid() == pid)
			return it;
		++it;
	}
	return it;
};

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

	Process::m_kernel_idle = new Process(0, {(void*)_kernel_idle_task, 0, ExecutableType::Flat});

	auto& regs = Process::m_kernel_idle->m_registers;
	regs.eax = 0xdeadc0de;
	regs.ebx = 0xbabecafe;
	regs.ecx = 0xdeadd00d;
	regs.edx = 0xbabadada;
	regs.esi = 0xdadadada;
	regs.edi = 0xdeaddead;

	new IRQSubscriber(0, [](const TrapFrame& frame){
		static unsigned ticksUntilSwitch = 20;
		if(ticksUntilSwitch-- == 0) {
			ticksUntilSwitch = 20;
			Scheduler::yield(frame);
		}
	});
}

/*
 *  Leaves the kernel and starts the scheduling loop
 */
void Scheduler::enter_scheduler_loop() {
	IRQDisabler disabler;

	Process::m_current = Process::m_kernel_idle;

	ready = true;

	Process::m_current->finalize_creation();
	Process::m_current->enter();
}

/*
 *  Picks the next process to execute and switches to it
 */
void Scheduler::yield(TrapFrame frame) {
	if(!ready) return;

	IRQDisabler disabler;

#ifdef SCHEDULE_LOG_DUMP_REG_SWITCH
	kdebugf("[Scheduler] yield, trap frame: \n");
	kdebugf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", frame.eax, frame.ebx, frame.ecx, frame.edx);
	kdebugf("ebp: %x, esp: %x, esi: %x, edi: %x\n", frame.ebp, frame.user_esp, frame.esi, frame.edi);
	kdebugf("eip: %x, CS: %x, EFLAGS: %x\n", frame.eip, frame.CS, frame.EFLAGS);
#endif

#ifdef SCHEDULE_LOG
	auto it = s_processes.begin();
	while(it != s_processes.end()) {
		auto* p = *it;
		kdebugf(" [%i] state: %s, cpl: %i\n", p->m_pid,
		        ((p->m_state == ProcessState::Ready) ? "Ready" :
		         (p->m_state == ProcessState::Leaving) ? "Leaving" :
		         (p->m_state == ProcessState::New ? "New" :
		         (p->m_state == ProcessState::Running ? "Running" : "undefined"))), (p->m_ring == Ring::CPL0 ? 0 : 3)
		);
		++it;
	}
#endif

	Process::m_current->save_regs_from_trap(frame);

	if(Process::m_current->m_state != ProcessState::Leaving)
		Process::m_current->m_state = ProcessState::Ready;

	auto* next_process = Scheduler::pick_next();
	while(Scheduler::handle_process_pick(next_process) != SchedulerAction::Use)
		next_process = Scheduler::pick_next();

	Process::m_current = next_process;
	Process::m_current->enter();
}

void Scheduler::notify_new_process(Process* v) {
	s_processes.push_back(v);
}

Process* Scheduler::pick_next() {
	if(s_processes.size() == 0)
		return Process::m_kernel_idle;
	else if(Process::m_current == Process::m_kernel_idle)
		return s_processes.front();
	else {
		auto pr = find(s_processes.begin(), s_processes.end(), Process::m_current->m_pid);
		pr = ++pr;
		if(pr != s_processes.end())
			return *pr;
		else
			return s_processes.front();
	}
}

SchedulerAction Scheduler::handle_process_pick(Process* next_process) {
	switch(next_process->m_state) {
		case ProcessState::New: {
			auto* old = Process::m_current;

			//  FIXME:  Cludge
			Process::m_current = next_process;
			if(next_process->finalize_creation())
				next_process->m_state = ProcessState::Ready;
			Process::m_current = old;

			return SchedulerAction::PickAgain;
		}
		case ProcessState::Ready:
			return SchedulerAction::Use;

		case ProcessState::Leaving: {
			kdebugf("[Scheduler] Cleaning up process PID %i \n", next_process->m_pid);

			auto it = find(s_processes.begin(), s_processes.end(), next_process->m_pid);
			if(it != s_processes.end()) {
				s_processes.erase(it);
			}

			it = find(Process::m_all_processes.begin(), Process::m_all_processes.end(), next_process->m_pid);
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
		case ProcessState::Frozen:
			kpanic();
		default:
			kpanic();
	}
}
