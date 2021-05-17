#pragma once
#include <Arch/i386/InactiveTaskFrame.hpp>
#include <Arch/i386/PtraceRegs.hpp>
#include <Kernel/Memory/Ptr.hpp>
#include <Kernel/Memory/VMRegion.hpp>
#include <Kernel/Process/ProcMem.hpp>
#include <Kernel/SystemTypes.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/SharedPtr.hpp>

enum class ProcessState {
	New,
	Ready,
	Running,
	Blocking,
	Leaving,
	Sleeping
};

struct ProcessFlags {
	uint8_t randomize_vm : 1;
	uint8_t kernel_thread : 1;
	uint8_t need_resched : 1;
	uint8_t uninterruptible : 1;
	uint32_t _reserved : 28;
} __attribute__((packed));
static_assert(sizeof(ProcessFlags) == sizeof(uint32_t), "ProcessFlags should be 4 bytes");

class PML4;
class VMapping;
template<class T> class UserPtr;
struct RunQueue;

using gen::Spinlock;
class Process {
	friend class Scheduler;

	template<class T>
	using List = gen::List<T>;
	template<class T>
	using SharedPtr = gen::SharedPtr<T>;
	using Mutex = gen::Spinlock;

	static Process* s_current;
	static List<Process*> s_process_list;
	static Mutex s_process_list_lock;

	//  !!!!!!!!!!!!!!!!!!!!!
	//  These MUST be the first 4 members of the class
	//  !!!!!!!!!!!!!!!!!!!!!
	[[maybe_unused]] InactiveTaskFrame* m_interrupted_task_frame;   //  Offset 0x0
	PhysPtr<PML4> m_pml4;                                           //  Offset 0x8
	void* m_kernel_stack_bottom;                                    //  Offset 0x10
	[[maybe_unused]] void* m_userland_stack;                                         //  Offset 0x18

	Mutex m_process_lock;
	pid_t m_pid;
	ProcessFlags m_flags;
	ProcessState m_state;
	ProcMem m_address_space;
	uint8_t m_priority;
	uint64_t m_quants_left;
	uint64_t m_preempt_count;
	uint64_t m_kernel_gs_base;

	//	List<Process*> m_children;

	Process(pid_t, ProcessFlags);
	~Process();

	/*
	 *  Task creation
	 */
	static Process* create(ProcessFlags flags, KOptional<pid_t> force_pid = {});
	static Process* create_idle_task(void(*)());
	static Process* create_userland_test();

	[[maybe_unused]] static void finalize_switch(Process* prev, Process* next);

	void* create_kernel_stack();
	void* create_user_stack();
	void* modify_kernel_stack_bootstrap(void* ret_address);
public:
	static Process* current();
	static Process* create_kernel_thread(void(*)());

	static void self_lock();
	static void self_unlock();

	void start();
	void set_state(ProcessState);
	void force_reschedule();
	void msleep(uint64_t);

	pid_t pid() const;
	ProcessState state() const;
	ProcessFlags flags() const;
	PhysPtr<PML4> pml4() const;
	uint8_t priority() const;
	InactiveTaskFrame* frame() const;
	ProcMem* memory();

	void preempt_enable();
	void preempt_disable();
	uint64_t preempt_count() const;

	/*
	 *  Syscalls - defined in ProcSyscalls.cpp
	 */
	static pid_t getpid();
	static uint64_t getpriority();
	static void klog(UserPtr<const char>);
};

//static_assert(__builtin_offsetof(Process, m_interrupted_task_frame) == 0);
//static_assert(__builtin_offsetof(Process, m_pml4) == 8);


