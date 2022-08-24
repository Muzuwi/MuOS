#include <Arch/x86_64/PIT.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Daemons/SysDbg/SysDbg.hpp>
#include <Debug/klogf.hpp>
#include <LibGeneric/String.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <SMP/SMP.hpp>

constexpr bool is_numeric(char ch) {
	return ch >= 0x30 && ch < 58;
}

constexpr bool is_valid_hex_char(char ch) {
	return ch >= 0x61 && ch < 0x67;
}

constexpr KOptional<uint64> parse_hex(gen::String const& string) {
	uint64 value = 0;
	bool any_chars = false;
	for(auto ch : string) {
		if(is_numeric(ch)) {
			any_chars = true;
			const auto nibble = static_cast<uint64>(ch - '0');
			value <<= 4u;
			value |= nibble;
		} else if(is_valid_hex_char(ch)) {
			any_chars = true;
			const auto nibble = static_cast<uint64>(10 + ch - 'a');
			value <<= 4u;
			value |= nibble;
		} else {
			return { gen::nullopt };
		}
	}
	if(!any_chars) {
		return { gen::nullopt };
	}
	return { value };
}

//  Very ad-hoc tests to get things working
static_assert(!parse_hex(gen::String("")).has_value());
static_assert(parse_hex(gen::String("0")).unwrap_or_default(9999999) == 0);
static_assert(parse_hex(gen::String("9")).unwrap_or_default(9999999) == 9);
static_assert(parse_hex(gen::String("a")).unwrap_or_default(9999999) == 0xa);
static_assert(!parse_hex(gen::String("babaX")).has_value());
static_assert(parse_hex(gen::String("1a")).unwrap_or_default(9999999) == 0x1a);
static_assert(parse_hex(gen::String("deadbabe")).unwrap_or_default(9999999) == 0xdeadbabe);
static_assert(parse_hex(gen::String("ffffffffd0000000")).unwrap_or_default(9999999) == 0xffffffffd0000000);

void SysDbg::handle_command(gen::List<gen::String> const& args) {
	auto thread = Thread::current();
	auto process = thread->parent();

	if(args.empty()) {
		return;
	}

	const auto& command = args.front();
	auto ptr = args.begin();
	ptr++;

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
	} else if(command == "xp" || command == "xpd") {
		//  No parameters passed
		if(ptr == args.end()) {
			return;
		}

		const auto maybe_address = parse_hex(*ptr);
		if(!maybe_address.has_value()) {
			return;
		}
		ptr++;
		//  Optional parameter - number of bytes to read.
		unsigned count = 1;
		if(ptr != args.end()) {
			const auto maybe_count = parse_hex(*ptr);
			if(maybe_count.has_value()) {
				count = maybe_count.unwrap();
			}
		}

		const auto address = maybe_address.unwrap();

		for(unsigned i = 0; i < count; ++i) {
			if(command == "xp") {
				const auto physical = PhysPtr<uint8>(reinterpret_cast<uint8*>(address + i));
				klogf("kdebugger({}): [{x}] = ", thread->tid(), address + i);

				const auto byte = *physical;

				klogf("[{x}]\n", byte);
			} else {
				const auto physical = PhysPtr<uint32>(reinterpret_cast<uint32*>(address + i * 4));
				klogf("kdebugger({}): [{x}] = ", thread->tid(), address + i * 4);

				const auto dword = *physical;

				klogf("[{x}]\n", dword);
			}
		}
	}
}

void SysDbg::sysdbg_thread() {
	klogf("kdebugger({}): started\n", Thread::current()->tid());
	gen::String current {};
	gen::List<gen::String> parameters {};
	while(true) {
		Serial::debugger_semaphore().wait();

		auto& buffer = Serial::buffer();
		while(!buffer.empty()) {
			const auto data = buffer.try_pop().unwrap();
			switch(data) {
				case 0x7f: {
					if(!current.empty()) {
						current.resize(current.size() - 1);
					} else {
						if(!parameters.empty()) {
							const auto last = parameters.back();
							parameters.pop_back();
							current = last;
						}
					}
					break;
				}
				case '\r': {
					if(!current.empty()) {
						parameters.push_back(current);
					}
					if(!parameters.empty()) {
						int i = 0;
						for(auto& param : parameters) {
							klogf("arg[{}] = '{}'\n", i++, param.data());
						}

						const auto start = PIT::milliseconds();
						handle_command(parameters);
						const auto end = PIT::milliseconds();

						klogf("kdebugger({}): Command handling took {}ms\n", Thread::current()->tid(), end - start);
					}
					current.clear();
					parameters.clear();
					break;
				}
				case ' ': {
					if(!current.empty()) {
						parameters.push_back(current);
						current.clear();
					}
					break;
				}
				default: {
					if(data < 0x80) {
						current.append(static_cast<char>(data));
					}
					break;
				}
			}

			klogf("kdebugger({}) #: ", Thread::current()->tid());
			for(auto& param : parameters) {
				klogf("{} ", param.data());
			}
			klogf("{}\n", current.data());
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
	klogf("Name {{'{}'}}\n", process->m_simple_name.data());
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
