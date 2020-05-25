#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <Arch/i386/GDT.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <Kernel/Symbols.hpp>
#include <LibGeneric/List.hpp>
#include <Kernel/Debug/kassert.hpp>
#include <Kernel/Memory/VMM.hpp>

//#define SCHEDULE_LOG
//#define SCHEDULE_LOG_DUMP_REG_SWITCH

gen::List<Process*> Process::m_all_processes {};
Process* Process::m_current = nullptr;
Process* Process::m_kernel_idle = nullptr;
static pid_t nextPID = 1;
static pid_t nextUserPID = 1000;

Process::Process(pid_t pid, void* entrypoint)
: m_maps() {
	IRQDisabler disabler;
	this->m_pid = pid;
	this->m_registers = {};
	this->m_registers.eip = (uint32_t) entrypoint;
	this->m_registers.CS = GDT::get_kernel_CS();
	this->m_registers.EFLAGS = 0x0002 | 0x0200;

	this->m_ring = Ring::CPL0;
	this->m_state = ProcessState::New;
	this->m_directory = VMM::get_directory();
}

Process::Process(pid_t pid, void (* entrypoint)(int, char**))
		: Process(pid, (void*) entrypoint) {}

/*
 *  Creates a kernel task process and returns its' PID
 */
pid_t Process::create(void* call) {
	IRQDisabler disabler;
	kdebugf("[Process] New kernel task, entry %x, pid %i\n", call, nextPID);

	auto process = new Process(nextPID, call);

	m_all_processes.push_back(process);
	Scheduler::notify_new_process(process);

	return nextPID++;
}

/*
 *  Creates a user process with defined entrypoint and returns its' PID
 */
pid_t Process::create_user(void* call) {
	IRQDisabler disabler;
	kdebugf("[Process] New userland process, entry %x, pid %i\n", call, nextUserPID);

	auto process = new Process(nextUserPID, call);
	process->m_registers.CS = GDT::get_user_CS();
	process->m_ring = Ring::CPL3;

	m_all_processes.push_back(process);
	Scheduler::notify_new_process(process);

	return nextUserPID++;
}

/*
 *  Wrappers for C++ lambdas
 */
pid_t Process::create(void (* call)()) {
	return Process::create((void*) call);
}

pid_t Process::create_user(void (* pFunction)()) {
	return Process::create_user((void*) pFunction);
}

/*
 *  Enters a process, sets up the stack addresses and everything else to context switch to this process
 */
void Process::enter() {
	assert(this == m_current);
	IRQDisabler disabler;

#ifdef SCHEDULE_LOG
	kdebugf("[Process] context switch to pid %i [resuming at %x, SP at %x], ring %i\n", m_pid, m_registers.eip, m_registers.user_esp, m_ring == Ring::CPL0 ? 0 : 3);
#endif

#ifdef SCHEDULE_LOG_DUMP_REG_SWITCH
	kdebugf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", m_registers.eax, m_registers.ebx, m_registers.ecx, m_registers.edx);
	kdebugf("ebp: %x, esp: %x, esi: %x, edi: %x\n", m_registers.ebp, m_registers.user_esp, m_registers.esi, m_registers.edi);
	kdebugf("eip: %x, CS: %x, EFLAGS: %x\n", m_registers.eip, m_registers.CS, m_registers.EFLAGS);
#endif

	m_current->m_state = ProcessState::Running;

	//  Restore FPU state
	asm volatile(
	"fxrstor %0"
	::"m"(m_current->fpu_state())
	);

	//  Reload selectors
	asm volatile(
	"mov %%ds, %0\n"
	"mov %%es, %0\n"
	"mov %%fs, %0\n"
	"mov %%gs, %0\n"
	::"r"(m_ring == Ring::CPL3 ? (GDT::get_user_DS() | 3) : GDT::get_kernel_DS())
	);

	void* ptr = (m_ring == Ring::CPL3) ? (void*)m_current->m_directory : TO_PHYS(m_current->m_directory);
	//  Reload CR3
	asm volatile(
	"mov %%cr3, %0\n"
	::"r"(ptr)
	);

	//  Switch to kernel task
	if (m_ring == Ring::CPL0) {
		asm volatile(
		"mov %%eax, %0\n"
		"mov %%ebx, %1\n"
		"mov %%ecx, %2\n"
		"mov %%edx, %3\n"
		"mov %%edi, %4\n"
		"mov %%esi, %5\n"

		"mov %%esp, %9\n"
		"add %%esp, 12\n"
		"push %8\n"
		"push %7\n"
		"push %6\n"
		"mov %%ebp, %10\n"
		"iret"
		:
		: ""(m_current->m_registers.eax), ""(m_current->m_registers.ebx), ""(m_current->m_registers.ecx), ""(m_current->m_registers.edx), ""(m_registers.edi), ""(m_current->m_registers.esi),
		""(m_current->m_registers.eip), ""(m_current->m_registers.CS), ""(m_current->m_registers.EFLAGS), ""(m_current->m_registers.handler_esp), ""(m_current->m_registers.ebp)
		: "eax", "ebx", "ecx", "edx", "edi", "esi"
		);
	}
		//  Switch to user process
	else {
		asm volatile(
		"mov %%esp, %2\n"
		//				"mov %%eax, %%0\n"
		"push %0\n" //  DS
		"push %2\n" //  SS
		//				"push %%eax\n"  //  SS

		"mov %%eax, %3\n"
		"mov %%ebx, %4\n"
		"mov %%ecx, %5\n"
		"mov %%edx, %6\n"
		"mov %%edi, %7\n"
		"mov %%esi, %8\n"
		"push %9\n" //  EFLAGS
		"push %1\n" //  CS
		"push %10\n"    //  EIP
		"mov %%ebp, %11\n"
		"iret\n"
		::""(GDT::get_user_DS() | 3), ""(GDT::get_user_CS() | 3),
		""(m_registers.user_esp),
		""(m_registers.eax), ""(m_registers.ebx), ""(m_registers.ecx),
		""(m_registers.edx), ""(m_registers.edi), ""(m_registers.esi), ""(m_registers.EFLAGS),
		""(m_registers.eip), ""(m_registers.ebp)
		: "eax", "ebx", "ecx", "edx", "edi", "esi"
		);
	}
}

/*
 *  Saves the TrapFrame associated with a process and the current fpu context
 */
void Process::save_regs_from_trap(TrapFrame frame) {
	m_registers = frame;
	//  FIXME: The FPU context might be getting corrupted by the kernel on its' way from the irq handler to here
	asm volatile("fxsave %0"
	: "=m"(m_current->fpu_state()));
}

/*
 *  Kills a process specified by a PID
 */
void Process::kill(pid_t pid) {
	auto find = [](gen::BidirectionalIterator<gen::List<Process*>> begin,
	               gen::BidirectionalIterator<gen::List<Process*>> end,
	               pid_t pid) -> gen::BidirectionalIterator<gen::List<Process*>> {
		auto it = begin;
		while (it != end) {
			if ((*it)->m_pid == pid)
				return it;
			++it;
		}
		return it;
	};

	auto it = find(m_all_processes.begin(), m_all_processes.end(), pid);
	if (it == m_all_processes.end()) {
		kerrorf("WTF? Tried killing non-existant process!");
		return;
	}

	kdebugf("[Process] Process PID %i state: Leaving\n", pid);
	(*it)->m_state = ProcessState::Leaving;
}

Process::~Process() {
	if (m_fpu_state)
		delete m_fpu_state;

	if (m_directory) {
		if((uint64_t)m_directory < (uint64_t)&_ukernel_virtual_offset) {
			kerrorf("[Process] FIXME: Leaked page for process page directory!\n");
		} else {
			delete m_directory;
		}
	}

	for(auto map : m_maps)
		delete map;
}

/*
 *  Finish process creation, allocate stack, create a page directory and fpu state buffer for the process
 */
bool Process::finalize_creation() {
	IRQDisabler disabler;

	kdebugf("[Process] Finalizing process %i creation\n", this->m_pid);

	this->m_fpu_state = (FPUState*) KMalloc::get().kmalloc_alloc(512, 16);
	for (size_t i = 0; i < 512; ++i)
		m_fpu_state->state[i] = 0;

	if(m_ring == Ring::CPL3) {
		this->m_directory = PageDirectory::create_for_user();
		this->m_registers.user_esp = (uint32_t) VMM::allocate_user_stack(16384);
	} else {
		this->m_registers.handler_esp = (uint32_t) &_ukernel_tasks_stack + 4096 * (nextPID);
	}

	return true;
}
