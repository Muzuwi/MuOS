#pragma once
#include <LibGeneric/List.hpp>
#include <LibGeneric/String.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Memory/VMM.hpp>
#include <Process/Thread.hpp>
#include <SystemTypes.hpp>
#include <Daemons/SysDbg/SysDbg.hpp>

enum ProcPriv {
	User,
	Kernel
};

struct ProcFlags {
	ProcPriv privilege;
	bool randomize_vm;

	static constexpr ProcFlags flags_for_kernel_proc() {
		return ProcFlags {
			.privilege = Kernel,
			.randomize_vm = true
		};
	}

	static constexpr ProcFlags flags_for_user_proc() {
		return ProcFlags {
				.privilege = User,
				.randomize_vm = true
		};
	}
};

class Thread;

using gen::List;
using gen::SharedPtr;

class Process {
	friend void SysDbg::sysdbg_thread();
	friend class Scheduler;
	friend class Thread;
	friend void SysDbg::dump_process(gen::SharedPtr<Process> process, size_t depth);

	pid_t m_pid;
	ProcFlags m_flags;
	VMM m_vmm;

	List<SharedPtr<Process>> m_children;
	List<SharedPtr<Thread>> m_threads;

	gen::String m_simple_name;

	void add_child(SharedPtr<Process> const&);
	void add_thread(SharedPtr<Thread> const&);
	Process(pid_t, gen::String, ProcFlags);

	static Process& _init_ref();
public:
	static SharedPtr<Process> create(gen::String name, ProcFlags flags);
	static SharedPtr<Thread> create_with_main_thread(gen::String name, SharedPtr<Process> parent, void(*kernel_exec)(), ProcFlags flags = ProcFlags::flags_for_kernel_proc());
	static Process& _kerneld_ref();
	static SharedPtr<Process> kerneld();
	static SharedPtr<Process> init();
	~Process();

	pid_t pid() const { return m_pid; }
	ProcFlags flags() const { return m_flags; }

	VMM& vmm() { return m_vmm; }

	// ------------
	//   Syscalls
	// ------------
	static pid_t getpid();
	static void klog(UserString str);
	static uint64 heap_alloc(size_t region_size);
};
