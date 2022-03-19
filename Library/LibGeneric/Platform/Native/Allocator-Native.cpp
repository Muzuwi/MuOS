#include <LibGeneric/Allocator.hpp>
#include <cstdlib>

void* gen::__platform_alloc(size_t n) {
	return std::malloc(n);
}

void gen::__platform_free(void* p, size_t n) {
	return std::free(p);
}
