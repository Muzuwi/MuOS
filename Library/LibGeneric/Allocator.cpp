#include <LibGeneric/Allocator.hpp>
#include <Memory/KHeap.hpp>

#ifdef __is_kernel_build__

void* gen::__kernel_alloc(size_t n) {
	return KHeap::instance().slab_alloc(n);
}

void gen::__kernel_free(void* p, size_t n) {
	KHeap::instance().slab_free(p, n);
}

#endif