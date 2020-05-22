#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Kernel/Memory/PageToken.hpp>

class PMM {
public:
	static PageToken* allocate_page_user();
	static PageToken* allocate_page_kernel();
	static void handle_multiboot_memmap(void* mmap);
};
