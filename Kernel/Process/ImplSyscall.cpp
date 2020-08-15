#include <Arch/i386/IRQDisabler.hpp>
#include <Arch/i386/Timer.hpp>
#include <Kernel/Debug/kassert.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <LibGeneric/Vector.hpp>

template bool Process::verify_read(bool*);
template bool Process::verify_read(uint32_t *);
template bool Process::verify_read(uint16_t *);

void Process::exit(int rc) {
	IRQDisabler disabler;
	auto* process = Process::m_current;
	process->set_state(ProcessState::Leaving);
	process->m_exit_code = rc;
	Scheduler::switch_task();
}

unsigned Process::sleep(unsigned int seconds) {
	IRQDisabler disabler;
	return Timer::sleep_for(seconds * 1000);
}

template<class T>
bool Process::verify_read(T* addr) {
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
bool Process::verify_write(T* addr) {
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

size_t Process::write(int fildes, const void* buf, size_t nbyte) {
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

pid_t Process::getpid() {
	auto* process = Process::current();
	return process->pid();
}
