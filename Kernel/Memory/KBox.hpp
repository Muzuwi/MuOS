#pragma once
#include <SystemTypes.hpp>
#include <Debug/klogf.hpp>
#include <Memory/KHeap.hpp>

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
		klogf("dbg: freeing box ptr={} of size={}\n", m_ptr, m_size);
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


