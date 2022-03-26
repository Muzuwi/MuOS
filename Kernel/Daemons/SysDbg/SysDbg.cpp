#include <Daemons/SysDbg/SysDbg.hpp>
#include <Debug/klogf.hpp>
#include <Device/Serial.hpp>
#include <LibGeneric/String.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <SMP/SMP.hpp>
#include "Device/PIT.hpp"
#include "Kernel/ksleep.hpp"

void SysDbg::sysdbg_thread() {
	auto handle_command = [](gen::String const& command) {
		auto thread = Thread::current();
		auto process = thread->parent();
		if(command == "dvm") {
			klogf("kdebugger({}): process vmapping dump\n", thread->tid());
			for(auto& mapping : process->vmm().m_mappings) {
				klogf("{} - {} [{}{}{}{}][{}]\n", Format::ptr(mapping->addr()), Format::ptr(mapping->end()),
				      (mapping->flags() & VM_READ) ? 'R' : '-', (mapping->flags() & VM_WRITE) ? 'W' : '-',
				      (mapping->flags() & VM_EXEC) ? 'X' : '-', (mapping->flags() & VM_KERNEL) ? 'K' : 'U',
				      (mapping->type() == MAP_SHARED) ? 'S' : 'P');
			}
		} else if(command == "dp") {
			klogf("kdebugger({}): Kernel-mode process tree dump\n", thread->tid());
			SysDbg::dump_process(Process::kerneld());
			klogf("kdebugger({}): User-mode process tree dump\n", thread->tid());
			SysDbg::dump_process(Process::init());
		} else if(command == "da") {
			klogf("kdebugger({}): Kernel heap allocator statistics\n", thread->tid());
			KHeap::instance().dump_stats();
		} else if(command == "ds") {
			klogf("kdebugger({}): Scheduler statistics\n", thread->tid());
			SMP::ctb().scheduler().dump_statistics();
		} else if(command == "dc") {
			klogf("kdebugger({}): attached CPUs\n", thread->tid());
			for(auto const& cpu : SMP::attached_aps()) {
				klogf("... CPU #{}, APIC ID={}\n", cpu->vid(), cpu->apic_id());
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
				auto start = PIT::milliseconds();
				handle_command(command);
				auto end = PIT::milliseconds();
				command += '\0';
				klogf("kdebugger({}): command '{}'\n", Thread::current()->tid(), (char const*)&command[0]);
				klogf("kdebugger({}): handling command took {}ms\n", Thread::current()->tid(), end - start);
				command.clear();
			} else if(data < 0x80) {
				command += static_cast<char>(data);
			}
		}
	}
}

void SysDbg::dump_process(gen::SharedPtr<Process> process, size_t depth) {
	if(!process) {
		return;
	}

	auto print_header = [depth, process] {
		klogf("... ");
		for(unsigned i = 0; i < depth; ++i) {
			klogf("    ");
		}
		klogf("Process({}): ", process->pid());
	};

	print_header();
	klogf("Name {{'{}'}}\n", process->m_simple_name.to_c_string());
	print_header();
	klogf("PML4 {{{}}}\n", Format::ptr(process->vmm().pml4().get()));
	print_header();
	klogf("Flags {{privilege({}), randomize_vm({})}}\n", process->flags().privilege == User ? "User" : "Kernel",
	      process->flags().randomize_vm);
	{
		print_header();
		klogf("VMM {{\n");
		print_header();
		klogf("... VMapping count {{{}}}\n", process->vmm().m_mappings.size());
		print_header();
		klogf("... Kernel-used pages {{{}}}\n", process->vmm().m_kernel_pages.size());
		print_header();
		klogf("}}\n");
	}

	auto state_str = [](TaskState state) -> char const* {
		switch(state) {
			case TaskState::New: return "New";
			case TaskState::Ready: return "Ready";
			case TaskState::Running: return "Running";
			case TaskState::Blocking: return "Blocking";
			case TaskState::Leaving: return "Leaving";
			case TaskState::Sleeping: return "Sleeping";
			default: return "Invalid";
		}
	};

	print_header();
	klogf("Threads {{\n");
	for(auto& thread : process->m_threads) {
		print_header();
		klogf("... Thread({}), SP{{{}}}, PML4{{{}}}, State{{{}}}, PreemptCount{{{}}}, Pri{{{}}}\n", thread->tid(),
		      Format::ptr(thread->m_kernel_stack_bottom), Format::ptr(thread->m_pml4.get()), state_str(thread->state()),
		      thread->preempt_count(), thread->priority());
	}
	print_header();
	klogf("}}\n");

	print_header();
	klogf("Children {{\n");
	for(auto& child : process->m_children) {
		dump_process(child, depth + 1);
	}
	print_header();
	klogf("}}\n");
}
