#pragma once
#include <Kernel/Memory/VMM.hpp>
#include <LibGeneric/BitMap.hpp>

#define QUICKMAP_VIRT_START (1022u * 0x400000u)

class QuickMap {
	unsigned m_index;
public:
	QuickMap(void* addr);
	~QuickMap();
	void* address();
};
