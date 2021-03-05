#include <Kernel/Process/Process.hpp>

pid_t Process::getpid() {
	return Process::current()->pid();
}

uint64_t Process::getpriority() {
	return Process::current()->priority();
}
