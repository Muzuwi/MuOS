#include <Daemons/SysDbg/SysDbg.hpp>
#include <LibGeneric/String.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Device/Serial.hpp>
#include <SMP/SMP.hpp>
#include <Debug/klogf.hpp>

void SysDbg::sysdbg_thread() {
	auto handle_command = [](gen::String const& command) {
		auto thread = Thread::current();
		auto process = thread->parent();
		if(command == "dvm") {
			klogf("kdebugger({}): process vmapping dump\n", thread->tid());
			for(auto& mapping : process->vmm().m_mappings) {
				klogf("{} - {} [{}{}{}{}][{}]\n",
				        Format::ptr(mapping->addr()),
				        Format::ptr(mapping->end()),
				        (mapping->flags() & VM_READ) ? 'R' : '-',
				        (mapping->flags() & VM_WRITE) ? 'W' : '-',
				        (mapping->flags() & VM_EXEC) ? 'X' : '-',
				        (mapping->flags() & VM_KERNEL) ? 'K' : 'U',
				        (mapping->type() == MAP_SHARED) ? 'S' : 'P'
				       );
			}
		} else if(command == "dp") {
			klogf("kdebugger({}): process info dump\n", thread->tid());
			klogf("kdebugger({}): running under process pid={}\n", thread->tid(), process->pid());
			klogf("kdebugger({}): process privilege={}\n", thread->tid(),
			        process->m_flags.privilege == Kernel ? "kernel-mode" : "user-mode");
			klogf("kdebugger({}): {} children\n", thread->tid(), process->m_children.size());
			klogf("kdebugger({}): {} threads\n", thread->tid(), process->m_threads.size());
			for(auto const& th : process->m_threads) {
				klogf("kdebugger({}): - thread tid={}\n", thread->tid(), th->tid());
			}

		}
	};

	klogf("kdebugger({}): started\n", Thread::current()->tid());
	gen::String command {};
	while(true) {
		Serial::debugger_semaphore().wait();

		auto& buffer = Serial::buffer();
		while(!buffer.empty()) {
			const auto data = buffer.try_pop().unwrap();
			klogf("kdebugger({}): received {x}\n", Thread::current()->tid(), data);
			if(data == '\r') {
				handle_command(command);
				command += '\0';
				klogf("kdebugger({}): command '{}'\n", Thread::current()->tid(), (char const*)&command[0]);
				command.clear();
			} else if(data < 0x80) {
				command += static_cast<char>(data);
			}
		}
	}
}
