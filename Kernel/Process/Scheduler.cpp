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

/*
 *  Initializes the kernel idle task
 */
void Scheduler::initialize() {
	IRQDisabler disabler;
	kdebugf("[Scheduler] Init kernel idle task\n");

	Process::m_kernel_idle = new Process(0, [](int, char**) {
		while(true) {
			asm volatile ("hlt");
		}
	});

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

//	uint32_t temp = 0;
//	asm volatile(
//	"mov %0, %%esp"
//	:
//	: ""(temp)
//	);
//
//	Process::m_current->m_registers.esp = temp;
//	Process::m_current->m_registers.ebp = temp;

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

#ifdef SCHEDULE_LOG
	kdebugf("[Scheduler] yield, trap frame: \n");
#endif

#ifdef SCHEDULE_LOG_DUMP_REG_SWITCH
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

	auto find = [](gen::BidirectionalIterator<gen::List<Process*>> begin, gen::BidirectionalIterator<gen::List<Process*>> end, pid_t pid) -> gen::BidirectionalIterator<gen::List<Process*>> {
		auto it = begin;
		while(it != end) {
			if((*it)->m_pid == pid)
				return it;
			++it;
		}
		return it;
	};

	if(s_processes.size() == 0)
		Process::m_current = Process::m_kernel_idle;
	else if(Process::m_current == Process::m_kernel_idle)
		Process::m_current = s_processes.front();
	else {
		auto pr = find(s_processes.begin(), s_processes.end(), Process::m_current->m_pid);
		pr = ++pr;
		if(pr != s_processes.end())
			Process::m_current = *pr;
		else
			Process::m_current = s_processes.front();
	}

	if(Process::m_current->m_state == ProcessState::New) {
		if(Process::m_current->finalize_creation())
			Process::m_current->m_state = ProcessState::Ready;
	}

	if(Process::m_current->m_state == ProcessState::Ready) {
		Process::m_current->enter();
	} else if (Process::m_current->m_state == ProcessState::Leaving){
		kdebugf("[Scheduler] Cleaning up process PID %i \n", Process::m_current->m_pid);

		auto it = find(s_processes.begin(), s_processes.end(), Process::m_current->m_pid);
		if(it != s_processes.end()) {
			s_processes.erase(it);
		} else {
			kpanic();
		}

		it = find(Process::m_all_processes.begin(), Process::m_all_processes.end(), Process::m_current->m_pid);
		if(it != Process::m_all_processes.end()) {
			Process::m_all_processes.erase(it);
		} else {
			kpanic();
		}

		delete Process::m_current;
		Process::m_current = Process::m_kernel_idle;
	} else {
		kpanic();
	}

	Process::m_current->enter();
}

void Scheduler::notify_new_process(Process* v) {
	s_processes.push_back(v);
}
