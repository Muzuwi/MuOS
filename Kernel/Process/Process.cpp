#include <Process/Process.hpp>

Process::Process(pid_t pid, gen::String name, ProcFlags flags)
    : m_pid(pid)
    , m_flags(flags)
    , m_vmm(*this)
    , m_simple_name(name)
    , m_children()
    , m_threads() {}

Process::~Process() {}

Process& Process::_init_ref() {
	static Process s_init {
		1, gen::String {           "init"   },
          ProcFlags { .privilege = User,.randomize_vm = true }
	};
	return s_init;
}

Process& Process::_kerneld_ref() {
	static Process s_kerneld {
		2, gen::String {          "kerneld" },
          ProcFlags { .privilege = Kernel,.randomize_vm = true }
	};
	return s_kerneld;
}

SharedPtr<Process> Process::init() {
	static gen::SharedPtr<Process> ptr { &_init_ref() };
	return ptr;
}

gen::SharedPtr<Process> Process::kerneld() {
	static gen::SharedPtr<Process> ptr { &_kerneld_ref() };
	return ptr;
}

void Process::add_child(SharedPtr<Process> const& child) {
	gen::LockGuard guard { m_process_struct_lock };
	m_children.push_back(child);
}

void Process::add_thread(SharedPtr<Thread> const& thread) {
	gen::LockGuard guard { m_process_struct_lock };
	m_threads.push_back(thread);
}
