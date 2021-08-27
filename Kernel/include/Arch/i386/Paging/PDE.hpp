#pragma once
#include <Arch/i386/Paging/PTE.hpp>

enum class FlagPDE : uint64_t {
	ExecuteDisable = 1ul << 63u,

	LargePage = 1 << 7u,
	Accessed = 1 << 5u,
	Cache = 1 << 4u,
	WriteThrough = 1 << 3u,
	User	= 1 << 2u,
	RW		= 1 << 1u,
	Present = 1 << 0u,
};

class PDE {
	uint64_t m_data;
public:
	bool get(FlagPDE) const;
	void set(FlagPDE, bool);

	PhysPtr<PT> table() const;
	void set_table(PhysPtr<PT> page_table);
};

class ProcMem;

class PD {
	friend class VMM;
	PDE m_entries[512];
public:
	const PDE& operator[](void*) const;
	PDE& operator[](void*);
};
