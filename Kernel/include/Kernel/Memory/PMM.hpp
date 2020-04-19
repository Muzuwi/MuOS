#pragma once
#include <stdint.h>
#include <stddef.h>

namespace PMM {
	uintptr_t* allocate_page();
	void handle_multiboot_memmap(uintptr_t *mmap);
};
