#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Arch/i386/Registers.hpp>
#include <Arch/i386/PageDirectory.hpp>
#include <LibGeneric/List.hpp>
#include <include/Arch/i386/TrapFrame.hpp>
#include <Kernel/Memory/VMapping.hpp>

enum class ProcessState {
	New,
	Ready,
	Running,
	Blocking,
	Leaving,
	Frozen
};

enum class Ring {
	CPL0,
	CPL3
};

struct FPUState {
	uint8_t state[512];
} __attribute((aligned(16)));

class Process {
	friend class VMM;
	friend class Scheduler;

	static gen::List<Process*> m_all_processes;
	static Process* m_current;
	static Process* m_kernel_idle;

	Ring m_ring;
	pid_t m_pid;
	TrapFrame m_registers;
	ProcessState m_state;
	FPUState* m_fpu_state;
	PageDirectory* m_directory;
	gen::List<VMapping*> m_maps;

	//  Spawns kernel task
	Process(pid_t, void (*entrypoint)(int argc, char** argv));
	Process(pid_t, void*);
	~Process();

	[[noreturn]] void enter();
	bool finalize_creation();

	FPUState& fpu_state() { return *m_fpu_state; }
	void save_regs_from_trap(TrapFrame frame);
//	[[noreturn]] void enter_cr3_from_trap(TrapFrame);
//	[[noreturn]] void enter_cr0_from_trap(TrapFrame);
public:
	Ring ring() const { return m_ring; }
	pid_t pid() const { return m_pid; }

	static pid_t create(void (*call));
	static pid_t create(void (*call)());
	static pid_t create_user(void (*pFunction)());
	static pid_t create_user(void (*call));
	static Process* current() { return Process::m_current; }
	static void kill(pid_t);

	PageDirectory* directory() { return m_directory; }
};
