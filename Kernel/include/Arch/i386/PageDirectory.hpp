#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Arch/i386/Page.hpp>
#include <Kernel/Debug/kdebugf.hpp>

#define GET_DIR(a) ((uint32_t)a >> 22)

enum class DirectoryFlag {
	LargePage = (1 << 7),
	Accessed  = (1 << 5),
	Cached    = (1 << 4),
	WriteThrough = (1 << 3),
	User      = (1 << 2),
	RW		  = (1 << 1),
	Present   = (1 << 0),
};


class PageDirectoryEntry {
	uint32_t m_data;
public:
	PageDirectoryEntry();
	PageDirectoryEntry(uint32_t* table_address, uint16_t flags);

	PageTable* get_table() const;
	uint16_t raw_flags() const;

	void set_table(PageTable*);

	bool get_flag(DirectoryFlag) const;
	void set_flag(DirectoryFlag, bool);
};


class PageDirectory {
	PageDirectoryEntry m_entries[1024];
public:
	bool table_present(uint32_t*) const;

	Page* get_page(uint32_t*) const;
	PageDirectoryEntry& get_entry(uint32_t*);

	void dump();

	void load_cr3();

	void create_table(uint32_t*);
} __attribute__((aligned(4096)));


static_assert(sizeof(PageDirectoryEntry) == sizeof(uint32_t), "Invalid PDE class size!");
static_assert(sizeof(PageDirectory) == 1024 * sizeof(uint32_t), "Invalid Page Directory size!");