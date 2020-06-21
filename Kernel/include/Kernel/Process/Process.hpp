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

enum class ExecutableType {
	Flat,
	ELF
};

struct ExecutableImage {
	void* m_base;
	size_t m_size;
	ExecutableType m_type;
	ExecutableImage(void* base, size_t size, ExecutableType type)
	: m_base(base), m_size(size), m_type(type) { }
};

struct FPUState {
	uint8_t state[512];

	inline void restore() {
		asm volatile("fxrstor %0"
		::"m"(*this)
		:"memory");
	}

	inline void store() {
		asm volatile("fxsave %0"
		: "=m"(*this)
		::"memory");
	}
} __attribute((aligned(16)));

class Process {
	friend class VMM;
	friend class Scheduler;
	friend class Syscall;

	static gen::List<Process*> m_all_processes;
	static Process* m_current;
	static Process* m_kernel_idle;

	Ring m_ring;
	pid_t m_pid;
	TrapFrame m_registers;
	ProcessState m_state;
	FPUState* m_fpu_state;
	PageDirectory* m_directory;
	ExecutableImage m_executable;
	gen::List<VMapping*> m_maps;

	Process(pid_t, ExecutableImage);
	~Process();

	[[noreturn]] void enter();
	bool finalize_creation();
	bool load_process_executable();

	bool load_flat_binary();
	bool load_ELF_binary();
	bool finalize_creation_for_user();
	bool finalize_creation_for_kernel();


	FPUState& fpu_state() { return *m_fpu_state; }
	void save_regs_from_trap(TrapFrame frame);
	void load_segment_registers();

	[[noreturn]] static void jump_to_trap_ring3(const TrapFrame&);
	[[noreturn]] static void jump_to_trap_ring0(const TrapFrame&);
public:
	Ring ring() const { return m_ring; }
	pid_t pid() const { return m_pid; }

	static pid_t create(void (*call));
	static pid_t create(void (*call)());
	static pid_t create_user(void (*pFunction)());
	static pid_t create_user(void (*call));

	static pid_t create_from_ELF(void* base, size_t size);

	static Process* current() { return Process::m_current; }
	static void kill(pid_t);

	PageDirectory* directory() { return m_directory; }
};
