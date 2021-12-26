#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Memory/KHeap.hpp>

/*
 *  RAII-wrapper class for kernel heap allocations of a specific type
 */
template<class T>
class KBox {
	void* m_ptr;
	size_t m_size;
public:
	KBox() noexcept
	: m_ptr(nullptr), m_size(0) {}

	KBox(KBox<T> const&) = delete;

	KBox(KBox<T>&& box) noexcept
	: m_ptr(box.m_ptr), m_size(box.m_size)
	{
	}

	explicit KBox(T* ptr) noexcept
	: m_ptr(reinterpret_cast<void*>(ptr)), m_size(sizeof(T))
	{
	}

	explicit KBox(void* rawptr, size_t size) noexcept
	: m_ptr(rawptr), m_size(size)
	{
	}

	~KBox()
	{
		kdebugf("dbg: freeing box ptr=%x%x of size=%i\n", (uint64_t)m_ptr >>32u, (uint64_t)m_ptr&0xffffffffu, m_size);
		KHeap::free(m_ptr, m_size);
	}

	T* operator->() {
		return reinterpret_cast<T*>(m_ptr);
	}

	T& operator*() {
		return *reinterpret_cast<T*>(m_ptr);
	}

	operator bool() const {
		return m_ptr != nullptr;
	}

	void* get() const {
		return m_ptr;
	}

};


