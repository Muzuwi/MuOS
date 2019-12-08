/*	
	Paging header
*/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum page_flags {
	PAGE_GLOBAL = 1 << 8,
	PAGE_DIRTY = 1 << 6,
	PAGE_ACCESSED = 1 << 5,
	PAGE_NOCACHE = 1 << 4,
	PAGE_WRITETHROUGH = 1 << 3,
	PAGE_PRIVILEGE = 1 << 2,
	PAGE_RW = 1 << 1,
	PAGE_PRESENT = 1 << 0
};

enum dir_flags {
	DIR_GLOBAL = 1 << 8,
	DIR_PAGESIZE = 1 << 7,
	DIR_ACCESSED = 1 << 5,
	DIR_NOCACHE = 1 << 4,
	DIR_WRITETHROUGH = 1 << 3,
	DIR_PRIVILEGE = 1 << 2,
	DIR_RW = 1 << 1,
	DIR_PRESENT = 1 << 0
};

namespace Paging {
	void init_paging();
	void allocate_page(void*, void*, bool=false);
	bool is_present(void*);
}