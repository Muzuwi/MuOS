#include "SysDbg.hpp"
#include "Library/LibGeneric/String.hpp"
#include "Kernel/Process/Process.hpp"
#include "Kernel/Process/Thread.hpp"
#include "Kernel/Device/Serial.hpp"
#include "Kernel/SMP/SMP.hpp"

void SysDbg::sysdbg_thread() {
	auto handle_command = [](gen::String const& command) {
		auto thread = Thread::current();
		auto process = thread->parent();
#define S(a) (uintptr_t)a>>32u, (uintptr_t)a&0xffffffffu
		if(command == "dvm") {
			kdebugf("kdebugger(%i): process vmapping dump\n", thread->tid());
			for(auto& mapping : process->vmm().m_mappings) {
				kdebugf("%x%x - %x%x [%c%c%c%c][%c]\n",
				        S(mapping->addr()),
				        S(mapping->end()),
				        (mapping->flags() & VM_READ) ? 'R' : '-',
				        (mapping->flags() & VM_WRITE) ? 'W' : '-',
				        (mapping->flags() & VM_EXEC) ? 'X' : '-',
				        (mapping->flags() & VM_KERNEL) ? 'K' : 'U',
				        (mapping->type() == MAP_SHARED) ? 'S' : 'P'
				       );
			}
		} else if(command == "dp") {
			kdebugf("kdebugger(%i): process info dump\n", thread->tid());
			kdebugf("kdebugger(%i): running under process pid=%i\n", thread->tid(), process->pid());
			kdebugf("kdebugger(%i): process privilege=%s\n", thread->tid(),
			        process->m_flags.privilege == Kernel ? "kernel-mode" : "user-mode");
			kdebugf("kdebugger(%i): %i children\n", thread->tid(), process->m_children.size());
			kdebugf("kdebugger(%i): %i threads\n", thread->tid(), process->m_threads.size());
			for(auto const& th : process->m_threads) {
				kdebugf("kdebugger(%i): - thread tid=%i\n", thread->tid(), th->tid());
			}

		}
	};

	kdebugf("kdebugger(%i): started\n", Thread::current()->tid());
	gen::String command {};
	while(true) {
		Serial::debugger_semaphore().wait();

		auto& buffer = Serial::buffer();
		while(!buffer.empty()) {
			const auto data = buffer.try_pop().unwrap();
			kdebugf("kdebugger(%i): received %x\n", Thread::current()->tid(), data, data);
			if(data == '\r') {
				handle_command(command);
				command += '\0';
				kdebugf("kdebugger(%i): command '%s'\n", Thread::current()->tid(), &command[0]);
				command.clear();
			} else if(data < 0x80) {
				command += static_cast<char>(data);
			}
		}
	}
}
