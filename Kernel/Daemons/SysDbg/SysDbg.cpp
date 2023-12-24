#include <Arch/x86_64/PIT.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <Daemons/SysDbg/SysDbg.hpp>
#include <LibGeneric/String.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include <stddef.h>
#include "Core/Error/Error.hpp"
#include "Core/IO/BlockDevice.hpp"
#include "Core/Object/Object.hpp"
#include "Core/Object/Tree.hpp"
#include "Memory/KHeap.hpp"
#include "SystemTypes.hpp"

CREATE_LOGGER("sysbdg", core::log::LogLevel::Debug);

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
		log.info("kdebugger({}): process vmapping dump", thread->tid());
		for(auto& mapping : process->vmm().m_mappings) {
			log.info("{} - {} [{}{}{}{}][{}]", Format::ptr(mapping->addr()), Format::ptr(mapping->end()),
			         (mapping->flags() & VM_READ) ? 'R' : '-', (mapping->flags() & VM_WRITE) ? 'W' : '-',
			         (mapping->flags() & VM_EXEC) ? 'X' : '-', (mapping->flags() & VM_KERNEL) ? 'K' : 'U',
			         (mapping->type() == MAP_SHARED) ? 'S' : 'P');
		}
	} else if(command == "dp") {
		log.info("kdebugger({}): Kernel-mode process tree dump", thread->tid());
		SysDbg::dump_process(Process::kerneld());
		log.info("kdebugger({}): User-mode process tree dump", thread->tid());
		SysDbg::dump_process(Process::init());
	} else if(command == "da") {
		log.info("kdebugger({}): Kernel heap allocator statistics", thread->tid());
		KHeap::instance().dump_stats();
	} else if(command == "ds") {
		log.info("kdebugger({}): Scheduler statistics", thread->tid());
		this_cpu()->scheduler->dump_statistics();
	} else if(command == "dc") {
		log.info("kdebugger({}): attached CPUs", thread->tid());
		//  for(auto const& cpu : SMP::attached_aps()) {
		//  log.info("... CPU #{}, APIC ID={}", cpu->vid(), cpu->apic_id());
		//  }
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
				log.info("kdebugger({}): [{x}] = ", thread->tid(), address + i);

				const auto byte = *physical;

				log.info("[{x}]", byte);
			} else {
				const auto physical = PhysPtr<uint32>(reinterpret_cast<uint32*>(address + i * 4));
				log.info("kdebugger({}): [{x}] = ", thread->tid(), address + i * 4);

				const auto dword = *physical;

				log.info("[{x}]", dword);
			}
		}
	} else if(command == "db") {
		log.info("kdebugger({}): Block devices:", thread->tid());
		(void)core::obj::for_each_object_of_type(core::obj::ObjectType::BlockDevice, [thread](core::obj::KObject* obj) {
			auto* bdev = reinterpret_cast<core::io::BlockDevice*>(obj);
			auto name = bdev->name();
			log.info("kdebugger({}):  - {}", thread->tid(), name.data());
		});
	} else if(command == "readblock") {
		//  No parameters passed
		if(ptr == args.end()) {
			return;
		}

		gen::String name = *ptr;
		ptr++;
		if(ptr == args.end()) {
			return;
		}

		const auto maybe_address = parse_hex(*ptr);
		if(!maybe_address.has_value()) {
			return;
		}
		ptr++;

		//  Optional parameter - number of sectors to read
		unsigned count = 1;
		if(ptr != args.end()) {
			const auto maybe_count = parse_hex(*ptr);
			if(maybe_count.has_value()) {
				count = maybe_count.unwrap();
			}
		}

		size_t blk_start = maybe_address.unwrap();
		size_t blk_count = count;
		struct {
			core::io::BlockDevice* blk;
			gen::String blk_name;
		} context { .blk = nullptr, .blk_name = name };

		(void)core::obj::for_each_object_of_type(core::obj::ObjectType::BlockDevice,
		                                         [&context](core::obj::KObject* obj) {
			                                         auto* bdev = reinterpret_cast<core::io::BlockDevice*>(obj);
			                                         if(obj->name() == context.blk_name) {
				                                         context.blk = bdev;
			                                         }
		                                         });
		if(!context.blk) {
			log.error("kdebugger({}): could not find blk with name: {}", thread->tid(), name.data());
			return;
		}

		size_t sector_size = 0;
		if(const auto err = context.blk->blksize(sector_size); err != core::Error::Ok) {
			log.error("kdebugger({}): could not read sector size, error={}", thread->tid(), static_cast<size_t>(err));
			return;
		}
		size_t sector_count = 0;
		if(const auto err = context.blk->blkcount(sector_count); err != core::Error::Ok) {
			log.error("kdebugger({}): could not read sector count, error={}", thread->tid(), static_cast<size_t>(err));
			return;
		}

		log.info("kdebugger({}): sectors={}, sector_size={}", thread->tid(), sector_count, sector_size);

		auto buf_size = sector_size * count;
		auto* buffer = (uint8*)KHeap::instance().chunk_alloc(buf_size);
		if(!buffer) {
			log.info("kdebugger({}): failed to allocate buffer of size", thread->tid(), buf_size);
			return;
		}

		if(const auto err = context.blk->read(gen::move(buffer), gen::move(buf_size), gen::move(blk_start),
		                                      gen::move(blk_count));
		   err != core::Error::Ok) {
			log.error("kdebugger({}): could not read sectors, error={}", thread->tid(), static_cast<size_t>(err));
			return;
		}

		log.info("{} {} {}", context.blk_name.data(), blk_start, blk_count);

		const size_t bytes_per_row = 16;

		for(auto i = 0; i < buf_size; i += bytes_per_row) {
			log.info("[kdebugger({})]: #{x} | ", thread->tid(), (blk_start * sector_size + i));
			for(auto j = i; (j < (i + bytes_per_row)) && (j < buf_size); j++) {
				log.info("{x} ", buffer[j]);
			}
			log.info("");
		}

		KHeap::instance().chunk_free(buffer);
		//  readblock ide@01f0:03f6 2 2
		//  readblock ide@01f0:03f6 80000 1
		//  readblock ide@01f0:03f6 7ffff 1
		//  readblock ide@01f0:03f6 7ffff 2
	}
}

void SysDbg::sysdbg_thread() {
	log.info("kdebugger({}): started", Thread::current()->tid());
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
							log.info("arg[{}] = '{}'", i++, param.data());
						}

						const auto start = PIT::milliseconds();
						handle_command(parameters);
						const auto end = PIT::milliseconds();

						log.info("kdebugger({}): Command handling took {}ms", Thread::current()->tid(), end - start);
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

			log.info("kdebugger({}) #: ", Thread::current()->tid());
			for(auto& param : parameters) {
				log.info("{} ", param.data());
			}
			log.info("{}", current.data());
		}
	}
}

void SysDbg::dump_process(gen::SharedPtr<Process> process, size_t depth) {
	if(!process) {
		return;
	}

	auto print_header = [depth, process] {
		log.info("... ");
		for(unsigned i = 0; i < depth; ++i) {
			log.info("    ");
		}
		log.info("Process({}): ", process->pid());
	};

	print_header();
	log.info("Name {{'{}'}}", process->m_simple_name.data());
	print_header();
	log.info("PML4 {{{}}}", Format::ptr(process->vmm().pml4().get()));
	print_header();
	log.info("Flags {{privilege({}), randomize_vm({})}}", process->flags().privilege == User ? "User" : "Kernel",
	         process->flags().randomize_vm);
	{
		print_header();
		log.info("VMM {{");
		print_header();
		log.info("... VMapping count {{{}}}", process->vmm().m_mappings.size());
		print_header();
		log.info("... Kernel-used pages {{{}}}", process->vmm().m_kernel_pages.size());
		print_header();
		log.info("}}");
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
	log.info("Threads {{");
	for(auto& thread : process->m_threads) {
		print_header();
		log.info("... Thread({}), SP{{{}}}, PML4{{{}}}, State{{{}}}, PreemptCount{{{}}}, Pri{{{}}}", thread->tid(),
		         Format::ptr(thread->m_kernel_stack_bottom), Format::ptr(thread->m_pml4.get()),
		         state_str(thread->state()), thread->preempt_count(), thread->priority());
	}
	print_header();
	log.info("}}");

	print_header();
	log.info("Children {{");
	for(auto& child : process->m_children) {
		dump_process(child, depth + 1);
	}
	print_header();
	log.info("}}");
}
