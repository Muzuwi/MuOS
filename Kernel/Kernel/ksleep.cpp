#include <Arch/i386/PIT.hpp>
#include <Kernel/ksleep.hpp>

void ksleep(uint64_t milliseconds) {
	auto start = PIT::milliseconds();
	while(PIT::milliseconds() - start < milliseconds)
		;
}
