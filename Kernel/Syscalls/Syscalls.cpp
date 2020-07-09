#include <Arch/i386/IRQDisabler.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Syscalls/SyscallList.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <LibGeneric/Vector.hpp>
#include <include/Kernel/Debug/kpanic.hpp>

uint32_t Syscall::dispatch(uint32_t function_id, const _SyscallParamPack& args) {
	ASSERT_IRQ_DISABLED();

	const auto syscall_number = static_cast<SyscallNumber>(function_id);
	switch (syscall_number) {
		case SyscallNumber::Exit: Syscall::exit(args.arg1);
		case SyscallNumber::Write:
			return Syscall::write(args.arg1, args.get<2, const void*>(), args.arg3);
		case SyscallNumber::Sleep:
			return Syscall::sleep(args.arg1);
		default:
			kerrorf("Process(%i): Invalid syscall function number\n", Process::current()->pid());
	}

	return 0x0;
}

void Syscall::exit(int retval) {
	ASSERT_IRQ_DISABLED();
	kdebugf("[Syscall] Process %i wants to close! Exit: %i\n", Process::current()->pid(), retval);
	Process::kill(Process::current()->pid());
	Scheduler::switch_task();
	kpanic();
}

unsigned Syscall::sleep(unsigned int seconds) {
	kdebugf("[Syscall] Process(%i): sleep\n", Process::current()->pid());
	return Process::sleep(seconds);
}

size_t Syscall::write(int fildes, const void* buf, size_t nbyte) {
	ASSERT_IRQ_DISABLED();
	if(fildes != 0)
		return -EBADF;
	if(!verify_read(reinterpret_cast<const uint8_t*>(buf)))
		return -EPERM;
	if(nbyte > 1024)
		return -EFBIG;

	gen::vector<uint8_t> bytes;
	bytes.resize(nbyte+1);

	//  FIXME:  Dangerous as hell, but right now i want to get something working
	for(unsigned i = 0; i < nbyte; ++i){
		bytes[i] = *((uint8_t*)(buf) + i);
	}
	bytes[nbyte] = '\0';

	kdebugf("Process(%i): %s\n", Process::current()->pid(), &bytes[0]);
	return nbyte;
}

template<class T>
bool Syscall::verify_read(T* addr) {
	ASSERT_IRQ_DISABLED();
	auto* cur = Process::current();
	for(const auto& map : cur->m_maps) {
		if((uintptr_t)addr >= (uintptr_t)map->addr() &&
		   (uintptr_t)addr + sizeof(T) < (uintptr_t)map->addr() + map->size()) {
			return (map->flags() & PROT_READ);
		}
	}
	return false;
}

template<class T>
bool Syscall::verify_write(T* addr) {
	ASSERT_IRQ_DISABLED();
	auto* cur = Process::current();
	for(const auto& map : cur->m_maps) {
		if((uintptr_t)addr >= (uintptr_t)map->addr() &&
		   (uintptr_t)addr + sizeof(T) < (uintptr_t)map->addr() + map->size()) {
			return (map->flags() & PROT_WRITE);
		}
	}
	return false;
}

void* Syscall::mmap(void* addr, size_t len, int prot, int flags, int, off_t) {
	if((uint64_t)addr >= (uint64_t)&_ukernel_virtual_offset)
		return reinterpret_cast<void*>(-EINVAL);

	if(!((flags & MAP_SHARED) || (flags & MAP_FIXED) || (flags & MAP_PRIVATE)))
		return reinterpret_cast<void*>(-EINVAL);

	if(len == 0)
		return reinterpret_cast<void*>(-EINVAL);

	return nullptr;

//	auto* process = Process::current();
//	auto* mapping = new VMapping(addr, len, prot, flags);
//	process->m_maps.push_back(mapping);

//	return ;
}
