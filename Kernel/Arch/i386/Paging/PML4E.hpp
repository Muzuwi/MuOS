#pragma once
#include <Arch/i386/Paging/PDPTE.hpp>

enum class FlagPML4E : uint64_t {
	ExecuteDisable = 1ul << 63u,

	Accessed = 1 << 5u,
	Cache = 1 << 4u,
	WriteThrough = 1 << 3u,
	User	= 1 << 2u,
	RW		= 1 << 1u,
	Present = 1 << 0u,
};

class PML4E {
	uint64_t m_data;
public:
	bool get(FlagPML4E) const;
	void set(FlagPML4E, bool);

	PhysPtr<PDPT> directory();
	void set_directory(PhysPtr<PDPT> page_directory_ptr_table);
};

class ProcMem;

class PML4 {
	friend class VMM;
	PML4E m_entries[512];
public:
	const PML4E& operator[](void*) const;
	PML4E& operator[](void*);
};