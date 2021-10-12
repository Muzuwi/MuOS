#pragma once
#include <LibGeneric/StdRequired.hpp>

//[[nodiscard]] void* operator new(size_t);
//[[nodiscard]] void* operator new[](size_t);
//[[nodiscard]] void* operator new (size_t count, std::align_val_t);
//[[nodiscard]] void* operator new[](size_t count, std::align_val_t);
//void operator delete(void*);
//void operator delete(void*, size_t);
//void operator delete[](void*, size_t);
//void operator delete[](void*);
//void operator delete(void* pointer, size_t, std::align_val_t);
//void operator delete[](void* pointer, size_t, std::align_val_t);

namespace KHeap {
	void init();
	void* allocate(size_t size);
	void free(void* ptr, size_t = 0);
	void dump_stats();
}