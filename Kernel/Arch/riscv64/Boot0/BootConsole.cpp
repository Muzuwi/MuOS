#include <Arch/riscv64/Boot0/BootConsole.hpp>

__attribute__((weak)) void bootcon::init(libfdt::FdtHeader const*) {
	//  Intentionally left empty
}

__attribute__((weak)) void bootcon::putch(char) {
	//  Intentionally left empty
}

__attribute__((weak)) void bootcon::remap() {
	//  Intentionally left empty
}
