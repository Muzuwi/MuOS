#pragma once
#include <stdint.h>
#include <stddef.h>

class PageToken;

class PMM {
public:
	static PageToken* allocate_page_user();
	static PageToken* allocate_page_kernel();
	static void handle_multiboot_memmap(void* mmap);
	static void free_page_from_token(PageToken*);
};
