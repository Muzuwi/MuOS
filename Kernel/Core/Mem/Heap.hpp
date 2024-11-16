#pragma once
#include <LibGeneric/Memory.hpp>
#include <SystemTypes.hpp>

namespace core::mem {
	static constexpr size_t HEAP_DEFAULT_SIZE = 32_MiB;

	/*	Allocate memory on the kernel heap.
	 *
	 * 	This is functionally equivalent to malloc. You can use this to
	 * 	dynamically allocate buffers of memory used by kernel code. Memory
	 * 	allocated using hmalloc must be deallocated after use by calling
	 * 	hfree.
	 *
	 * 	Note that the global kernel heap is currently limited to a size
	 *  specified by HEAP_DEFAULT_SIZE.
	 */
	void* hmalloc(size_t n);

	/*	Free memory previously allocated using hmalloc.
	 */
	void hfree(void*);

	/*  Allocate an object of type T using hmalloc and construct it.
	 *  The object is constructed in-place by forwarding the arguments
	 *  passed in the call to this function.
	 */
	template<class T, class... Args>
	static inline T* make(Args&&... args) {
		auto* storage = hmalloc(sizeof(T));
		if(!storage) {
			return nullptr;
		}
		auto* obj = reinterpret_cast<T*>(storage);
		return gen::construct_at(obj, gen::forward<Args>(args)...);
	}
}