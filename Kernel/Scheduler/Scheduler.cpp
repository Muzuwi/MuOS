#include <Arch/i386/CPU.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Arch/i386/PIT.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Scheduler/RunQueue.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <LibGeneric/Algorithm.hpp>
#include <LibGeneric/Mutex.hpp>
#include <LibGeneric/LockGuard.hpp>

static Process* s_kernel_idle {};
static RunQueue s_rq {};
static gen::Mutex s_rq_lock {};

[[noreturn]] void _kernel_idle_task() {
	while(true)
		asm volatile("hlt");
}


void creat_test() {
	kdebugf("Hello from other!\n");
	uint64_t v {};
	while (true) {
		v = PIT::milliseconds();
		Process::current()->msleep(1500);
		kdebugf("3=%i\n", PIT::milliseconds() - v);
	}
}

void hello() {
	kdebugf("Hello world from kthread!\n");
	uint64_t v {};
	unsigned i = 4;
	while (true) {
		v = PIT::milliseconds();
		Process::current()->msleep(500);
		kdebugf("1=%i\n", PIT::milliseconds() - v);
		if(i - 1 == 0) {
			auto* p = Process::create_kernel_thread(creat_test);
			p->start();
		}
		if(i)
			i--;
	}
}

void hello2() {
	kdebugf("Hello world from kthread2!\n");
	uint64_t v {};
	while (true) {
		v = PIT::milliseconds();
		Process::current()->msleep(1000);
		kdebugf("2=%i\n", PIT::milliseconds() - v);
	}
}

static uint64_t _dummy_val {0};

/*
 *  Initialize the scheduler and enter the idle task
 */
void Scheduler::init() {
	CPU::irq_disable();

	kdebugf("[Scheduler] Init idle task\n");
	s_kernel_idle = Process::create_idle_task(_kernel_idle_task);

	kdebugf("[Scheduler] Init kernel_init\n");
	auto* kernel_init = Process::create_kernel_thread(hello);
	kernel_init->start();

	auto* test2 = Process::create_kernel_thread(hello2);
	test2->start();

	auto hndl = [](PtraceRegs*) {
		auto b = in(0x60);
		(void)b;
	};

	auto hndl_mouse = [](PtraceRegs*) {
		auto b = in(0x60);
		(void)b;
	};

	IRQDispatcher::register_handler(1, hndl);
	IRQDispatcher::register_handler(12, hndl_mouse);

	auto* userland = Process::create_userland_test();
	userland->start();

	kdebugf("[Scheduler] Enter idle task\n");
	//  Huge hack - only first 8 bytes of prev Process struct are used (only written), so use a dummy memory address
	//  to prevent null deref.
	CPU::switch_to(reinterpret_cast<Process*>(&_dummy_val), s_kernel_idle);
}

void Scheduler::tick() {
	if(!Process::current())
		return;

	if(Process::current() == s_kernel_idle) {
		//  Force reschedule when new threads appear
		if(rq_find_first_runnable() != nullptr)
			Process::current()->m_flags.need_resched = 1;
		else
			Process::current()->m_flags.need_resched = 0;
		return;
	}

	auto* process = Process::current();
	if(process->m_quants_left) {
		process->m_quants_left--;
	} else {
		rq_process_expire(process);
	}
}

void Scheduler::schedule() {
	//  Swap runqueues when all processes have expired within the active queue
	if(rq_find_first_runnable() == nullptr) {
		gen::swap(s_rq.m_active, s_rq.m_expired);
	}

	auto* next = rq_find_first_runnable();
	//  No runnable processes found, switch to idle
	if(!next)
		next = s_kernel_idle;

	CPU::switch_to(Process::current(), next);
}

void Scheduler::rq_process_expire(Process* process) {
	//  Move the process from the active queue to the expired queue
	s_rq.m_active->remove_process(process);
	s_rq.m_expired->add_process(process);
	process->m_flags.need_resched = 1;
	process->m_quants_left = pri_to_quants((!process->flags().kernel_thread ? 120 : 0) + process->priority());
}

unsigned Scheduler::pri_to_quants(uint8_t priority) {
	assert(priority <= 140);
//	return (140 - priority) * 4;
	if(priority < 120)
		return (140 - priority) * 20;
	else
		return (140 - priority) * 5;
}

Process* Scheduler::rq_find_first_runnable() {
	for(auto& pri_list : s_rq.m_active->m_lists) {
		if(pri_list.empty()) continue;
		for(auto& process : pri_list) {
			if(process->state() == ProcessState::Ready)
				return process;
		}
	}

	return nullptr;
}


void Scheduler::interrupt_return_common() {
	if(!Process::current())
		return;

	if(!Process::current()->flags().need_resched)
		return;

	Process::current()->m_flags.need_resched = 0;
	schedule();
}

void Scheduler::notify_process_start(Process* process) {
	gen::LockGuard<Mutex> lock {s_rq_lock};
	s_rq.m_active->add_process(process);
}

