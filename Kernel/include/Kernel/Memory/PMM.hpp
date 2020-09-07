#pragma once
#include <stdint.h>
#include <stddef.h>
#include <LibGeneric/List.hpp>
#include <LibGeneric/SharedPtr.hpp>

class PageToken;
class PRegion;

namespace PMM {
	gen::SharedPtr<PageToken> allocate_page_user();
	gen::SharedPtr<PageToken> allocate_page_kernel();
	void handle_multiboot_memmap(void* mmap);
	void free_page_from_token(PageToken*);
};
