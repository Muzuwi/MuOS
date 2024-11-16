#include <Core/Assert/Panic.hpp>
#include <LibGeneric/Allocator.hpp>

void* gen::__platform_alloc(size_t) {
	core::panic("gen::__platform_alloc not implemented!");
}

void gen::__platform_free(void*, size_t) {
	core::panic("gen::__platform_free not implemented!");
}
