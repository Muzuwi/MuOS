#pragma once
#include <Arch/x86_64/Paging/PDE.hpp>

enum class FlagPDPTE : uint64_t {
	ExecuteDisable = 1ul << 63u,

	HugePage = 1 << 7u,
	Accessed = 1 << 5u,
	Cache = 1 << 4u,
	WriteThrough = 1 << 3u,
	User = 1 << 2u,
	RW = 1 << 1u,
	Present = 1 << 0u,
};

class PDPTE {
	uint64_t m_data;
public:
	bool get(FlagPDPTE) const;
	void set(FlagPDPTE, bool);

	PhysPtr<PD> directory();
	void set_directory(PhysPtr<PD> page_directory);
};

class ProcMem;

class PDPT {
	friend class VMM;
	PDPTE m_entries[512];
public:
	const PDPTE& operator[](void*) const;
	PDPTE& operator[](void*);
};