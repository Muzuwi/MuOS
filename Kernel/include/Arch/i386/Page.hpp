#pragma once
#include <stdint.h>

#define GET_PAGE(a) (((uint32_t)a >> 12) & 0x03FF)

enum class PageFlag {
	Global = 1 << 8,
	Dirty = 1 << 6,
	Accessed = 1 << 5,
	Cache = 1 << 4,
	WriteThrough = 1 << 3,
	User	= 1 << 2,
	RW		= 1 << 1,
	Present = 1 << 0,
};

class Page {
	uint32_t m_data;
public:
	Page();

	void set_physical(uintptr_t*);
	bool get_flag(PageFlag) const;
	void set_flag(PageFlag, bool);
	uint32_t raw() const;
};


class PageTable {
	Page m_pages[1024];
public:
	PageTable();

	Page& get_page(uint32_t*);
} __attribute__((aligned(4096)));

void invlpg(uintptr_t*);
