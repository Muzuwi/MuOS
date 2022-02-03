#pragma once
#include <Memory/Ptr.hpp>

enum class FlagPTE : uint64_t {
	ExecuteDisable = 1ul << 63u,
	Global = 1 << 8u,
	PAT = 1 << 7u,
	Dirty = 1 << 6u,
	Accessed = 1 << 5u,
	Cache = 1 << 4u,
	WriteThrough = 1 << 3u,
	User	= 1 << 2u,
	RW		= 1 << 1u,
	Present = 1 << 0u,
};

class PTE {
	uint64_t m_data;
public:
	bool get(FlagPTE) const;
	void set(FlagPTE, bool);

	void set_page(PhysAddr);
};

class ProcMem;

class PT {
	friend class VMM;
	PTE m_entries[512];
public:
	const PTE& operator[](void*) const;
	PTE& operator[](void*);
};
