#include <LibGeneric/Allocator.hpp>
#include <Memory/KHeap.hpp>

void* gen::__platform_alloc(size_t n) {
	return KHeap::instance().slab_alloc(n);
}

void gen::__platform_free(void* p, size_t n) {
	KHeap::instance().slab_free(p, n);
}
