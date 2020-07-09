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
	Sleeping
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
	ProcessState m_state;
	FPUState* m_fpu_state;
	PageDirectory* m_directory;
	ExecutableImage m_executable;
	gen::List<VMapping*> m_maps;

	int m_exit_code;
	TrapFrame* m_current_irq_trap_frame;
	void* m_kernel_stack_bottom;
	bool m_is_finalized;

	Process(pid_t, Ring, ExecutableImage);
	~Process();

	[[noreturn]] void finalize();
	[[noreturn]] void _finalize_internal();
	[[noreturn]] void _finalize_for_user();
	[[noreturn]] void _finalize_for_kernel();

	bool load_process_executable(TrapFrame&);
	bool load_flat_binary(TrapFrame&);
	bool load_ELF_binary(TrapFrame&);

	PageDirectory* ensure_directory();
	void* ensure_kernel_stack();

	FPUState& fpu_state() { return *m_fpu_state; }
public:
	Ring ring() const { return m_ring; }
	pid_t pid() const { return m_pid; }
	void* irq_trap_frame() { return m_current_irq_trap_frame; }
	bool is_finalized() const { return m_is_finalized; }
	PageDirectory* directory() { return m_directory; }

	void set_state(ProcessState v);

	void wake_up();

	static Process* current() { return Process::m_current; }
	static pid_t create(void (*call));
	static pid_t create(void (*call)());
	static pid_t create_from_ELF(void* base, size_t size);
	static void kill(pid_t);

	/*
	 *  Functions that affect the currently running process
	 */
	[[noreturn]] static void exit(int rc);
	static unsigned sleep(unsigned seconds);
};
