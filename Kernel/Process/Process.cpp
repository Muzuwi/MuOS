#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <Arch/i386/GDT.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <Kernel/Symbols.hpp>
#include <LibGeneric/List.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Debug/kassert.hpp>
#include <Kernel/Memory/QuickMap.hpp>
#include <LibGeneric/ELFParser.hpp>
#include <Arch/i386/CPU.hpp>
#include <string.h>
#include <LibGeneric/Algorithm.hpp>
#include <Arch/i386/TrapFrame.hpp>
#include <include/Arch/i386/Timer.hpp>


gen::List<Process*> Process::m_all_processes {};
Process* Process::m_current = nullptr;
Process* Process::m_kernel_idle = nullptr;
static pid_t nextPID = 1;
static pid_t nextUserPID = 1000;

Process::Process(pid_t pid, Ring ring, ExecutableImage image)
: m_executable(image), m_maps() {
	this->m_state = ProcessState::New;
	this->m_ring = ring;
	this->m_pid = pid;
	this->m_exit_code = 0;

	this->m_kernel_stack_bottom = nullptr;
	this->m_current_irq_trap_frame = nullptr;
	this->m_directory = nullptr;
	this->m_fpu_state = nullptr;
	this->m_is_finalized = false;
}

Process::~Process() {
	ASSERT_IRQ_DISABLED();

	if (m_fpu_state)
		delete m_fpu_state;

	if (m_directory) {
		kerrorf("[Process] FIXME: Leaked page for process page directory!\n");
	}

	for(auto map : m_maps)
		delete map;
}

/*
 *  Creates a kernel task process and returns its' PID
 */
pid_t Process::create(void* call) {
	IRQDisabler disabler;
	kdebugf("[Process] New kernel task, entry %x, pid %i\n", call, nextPID);

	auto process = new Process(nextPID, Ring::CPL0, {call, 0, ExecutableType::Flat});

	m_all_processes.push_back(process);
	Scheduler::notify_new_process(process);

	return nextPID++;
}

/*
 *  Wrapper for C++ lambdas
 */
pid_t Process::create(void (* call)()) {
	return Process::create((void*) call);
}


pid_t Process::create_from_ELF(void* base, size_t size) {
	if(!base) return -1;
	IRQDisabler disabler;

	auto process = new Process(nextUserPID, Ring::CPL3,  {base, size, ExecutableType::ELF});
	kdebugf("[Process] New userland process, from ELF, pid %i\n", nextUserPID);

	m_all_processes.push_back(process);
	Scheduler::notify_new_process(process);

	return nextUserPID++;
}

/*
 *  Kills a process specified by a PID
 */
void Process::kill(pid_t pid) {
	IRQDisabler disabler;

	auto it = gen::find_if(m_all_processes, [&](Process* proc) -> bool {
		return proc->m_pid == pid;
	});
	if (it == m_all_processes.end()) {
		kerrorf("WTF? Tried killing non-existant process!");
		return;
	}

	kdebugf("[Process] Process PID %i state: Leaving\n", pid);
	(*it)->set_state(ProcessState::Leaving);
}

/*
 *  Finish process creation, allocate stack, create a page directory and fpu state buffer for the process
 */
__attribute__((used))
void Process::_finalize_internal() {
	ASSERT_IRQ_DISABLED();
	kdebugf("[Process] Finalizing process %i creation\n", this->m_pid);

	this->m_fpu_state = (FPUState*) KMalloc::get().kmalloc_alloc(512, 16);
	for (size_t i = 0; i < 512; ++i)
		m_fpu_state->state[i] = 0;

	if(m_ring == Ring::CPL0) {
		_finalize_for_kernel();
	} else {
		_finalize_for_user();
	}
}

void Process::_finalize_for_user() {
	ASSERT_IRQ_DISABLED();

	auto task_personal_stack = (uint32_t) VMM::allocate_user_stack(16384);

	TrapFrame frame {};
	frame.eip = 0xdeaddead;
	frame.eax = 0xaaaaaaaa;
	frame.ecx = 0xcccccccc;
	frame.edx = 0xdddddddd;
	frame.ebx = 0xbbbbbbbb;
	frame.user_esp = task_personal_stack;
	frame.ebp = 0xbdbdbdbd;
	frame.esi = 0x0d0d0d0d;
	frame.edi = 0xd0d0d0d0;
	frame.CS = GDT::get_user_CS() | 3u;
	frame.user_SS = GDT::get_user_DS() | 3u;
	frame.EFLAGS = 0x202;

	assert(this->load_process_executable(frame));
	CPU::load_segment_registers_for(Ring::CPL3);
	m_is_finalized = true;
	m_state = ProcessState::Running;
	CPU::jump_to_trap_ring3(frame);
}

void Process::_finalize_for_kernel() {
	ASSERT_IRQ_DISABLED();

	auto task_personal_stack = (uint32_t) VMM::allocate_kerneltask_stack();

	TrapFrame frame {};
	frame.eip = (uint32_t)m_executable.m_base;
	frame.eax = 0xaaaaaaaa;
	frame.ecx = 0xcccccccc;
	frame.edx = 0xdddddddd;
	frame.ebx = 0xbbbbbbbb;
	frame.user_esp = task_personal_stack;
	frame.ebp = 0xbdbdbdbd;
	frame.esi = 0x0d0d0d0d;
	frame.edi = 0xd0d0d0d0;
	frame.CS = GDT::get_kernel_CS();
	frame.user_SS = GDT::get_kernel_DS();
	frame.EFLAGS = 0x202;

	assert(this->load_process_executable(frame));
	CPU::load_segment_registers_for(Ring::CPL0);
	m_is_finalized = true;
	m_state = ProcessState::Running;
	CPU::jump_to_trap_ring0(frame);
}

bool Process::load_ELF_binary(TrapFrame& frame) {
	ASSERT_IRQ_DISABLED();
#ifdef PROCESS_DEBUG_ELF_PARSING
	kdebugf("[Process] Loading ELF binary, base: %x, size: %i\n", m_executable.m_base, m_executable.m_size);
#endif

	auto* parser = ELFParser32::from_image(m_executable.m_base, m_executable.m_size);
	if(!parser) {
		kerrorf("Process(%i): Invalid ELF binary\n", m_pid);
		return false;
	}

	const auto prog_headers = parser->program_headers();

	const void* file_base  = m_executable.m_base;
	const size_t file_size = m_executable.m_size;

	constexpr auto round_to_page_size = [](size_t size) -> size_t {
		return (size + 0x1000) & ~0xFFF;
	};

	const auto within_executable = [&](void* addr) -> bool {
		return (addr >= file_base) && (addr < (void*)((uintptr_t)file_base+file_size));
	};

	for(unsigned i = 0; i < prog_headers.size(); ++i) {
		const auto& header = prog_headers[i];
		if(header.p_type == SegType::Load) {
#ifdef PROCESS_LOG_ELF_CREATION
			kdebugf("[Process] Segment offset: %x, size: %i (rounded to %i), virt: %x\n",
			        header.p_offset,
			        header.p_memsz,
			        round_to_page_size(header.p_memsz),
			        header.p_vaddr);
#endif

			if(header.p_align != 0x1000) {
				kerrorf("[Process] Unsupported segment alignment!\n");
				continue;
			}

			const bool readable = (header.p_flags & PermFlags::Read);
			const bool writable = (header.p_flags & PermFlags::Write);
			const bool executable = (header.p_flags & PermFlags::Execute);

			const int flags = (readable ? PROT_READ : 0)  |
							  (writable ? PROT_WRITE : 0) |
						      (executable ? PROT_EXEC : 0);

			auto& mapping = VMapping::create_for_user(
										(void*)(header.p_vaddr & ~0xFFF), round_to_page_size(header.p_memsz),
			                            flags,
			                            MAP_SHARED);

			void* file_position = (void*)((uintptr_t)file_base + header.p_offset);
			if(!within_executable(file_position))
				return false;
			if(!within_executable((void*)((uintptr_t)file_position + header.p_filesz)))
				return false;

			unsigned copy_size = header.p_filesz;
			for(auto& pages : mapping.pages()) {
				void* phys_addr = pages->address();
				QuickMap mapper {phys_addr};

				void* destination = (void*)((uintptr_t)mapper.address() + (header.p_vaddr & 0xFFF));
				const void* source = (void*)((uintptr_t)file_position + (header.p_filesz - copy_size));
#ifdef PROCESS_LOG_ELF_CREATION
				kdebugf("[Process] Copying %x <- %x, size: %i\n", destination, source, copy_size);
#endif
				memcpy(destination, source, copy_size);

				if(copy_size < 0x1000)
					break;
				copy_size -= 0x1000;
			}
		}
	}

	frame.eip = (uint32_t)parser->entrypoint();

	delete parser;
	return true;
}

bool Process::load_flat_binary(TrapFrame& frame) {
	ASSERT_IRQ_DISABLED();
	if(m_executable.m_size == 0) return true;

#ifdef PROCESS_LOG_BIN_CREATION
	kdebugf("[Process] Loading flat binary, size: %i\n", m_executable.m_size);
#endif

	auto& mapping = VMapping::create_for_user((void*)0x100000, m_executable.m_size, PROT_READ | PROT_WRITE, MAP_SHARED);

	unsigned left = m_executable.m_size;
	for(auto& page : mapping.pages()) {
		QuickMap map{page->address()};

		unsigned copy_size = (left > 4096) ? left - 4096 : left;

#ifdef PROCESS_LOG_BIN_CREATION
		kdebugf("Copying %x -> %x\n", m_executable.m_base, page->address());
#endif

		memcpy(map.address(), m_executable.m_base, copy_size);
	}

	frame.eip = 0x100000;

	return true;
}

bool Process::load_process_executable(TrapFrame& frame) {
	ASSERT_IRQ_DISABLED();
	if(m_executable.m_type == ExecutableType::Flat)
		return load_flat_binary(frame);
	else
		return load_ELF_binary(frame);
}

/*
 *  Called by the task switch assembly entrypoint
 */
__attribute__((used))
PageDirectory* Process::ensure_directory() {
	ASSERT_IRQ_DISABLED();
	if(m_ring == Ring::CPL0)
		m_directory = PageDirectory::create_for_kernel();
	else
		m_directory = PageDirectory::create_for_user();

	kdebugf("Process(%i) received directory at physical %x\n", m_pid, m_directory);

	return m_directory;
}

/*
 *  Called by the task switch assembly entrypoint
 */
__attribute__((used))
void* Process::ensure_kernel_stack() {
	ASSERT_IRQ_DISABLED();
	m_kernel_stack_bottom = (void*)(VMM::allocate_interrupt_stack());
	return m_kernel_stack_bottom;
}


void Process::set_state(ProcessState v) {
	auto state_str = [](ProcessState state) -> const char* {
		switch (state) {
			case ProcessState::New:
				return "New";
			case ProcessState::Ready:
				return "Ready";
			case ProcessState::Leaving:
				return "Leaving";
			case ProcessState::Blocking:
				return "Blocking";
			case ProcessState::Running:
				return "Running";
			case ProcessState::Sleeping:
				return "Sleeping";
			default:
				return "invalid";
		}
	};

//	kdebugf("Process(%i) state changed: %s -> %s\n", m_pid, state_str(m_state), state_str(v));
	m_state = v;
}

void Process::wake_up() {
	assert(m_state == ProcessState::Sleeping);
	m_state = ProcessState::Ready;
}
