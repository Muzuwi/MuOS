#pragma once
#include <Kernel/Memory/Ptr.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <LibGeneric/BitMap.hpp>

#define QUICKMAP_VIRT_START (1022u * 0x400000u)

class QuickMap {
	unsigned m_index;
public:
	QuickMap(void* physical_address);

	template<class T>
	QuickMap(PhysPtr<T> addr)
	: QuickMap(static_cast<void*>(addr.get())) {}

	~QuickMap();
	void* address();
};
