#include <Core/Assert/Panic.hpp>
#include <Core/Mem/Heap.hpp>
#include <LibGeneric/Allocator.hpp>

void* gen::__platform_alloc(size_t n) {
	return core::mem::hmalloc(n);
}

void gen::__platform_free(void* ptr, size_t) {
	return core::mem::hfree(ptr);
}
