#pragma once
#include <Kernel/Symbols.hpp>
#include <Kernel/Memory/PageToken.hpp>

template<class T>
class PhysPtr {
	T* m_ptr;
public:
	explicit PhysPtr(T* ptr) noexcept
	: m_ptr(ptr) {}

	PhysPtr(const PageToken& token) noexcept
	: m_ptr(reinterpret_cast<T*>(token.address())) {}   //  FIXME: I don't like this cast

	T* get() {
		return m_ptr;
	}

	const T* get() const {
		return m_ptr;
	}

	PhysPtr& operator=(T* ptr) {
		m_ptr = ptr;
		return *this;
	}

	T* operator*() {
		auto* identity = reinterpret_cast<uint8_t*>(&_ukernel_identity_start);
		auto offset = reinterpret_cast<uint64_t>(m_ptr);

		return reinterpret_cast<T*>(identity+offset);
	}

	T const* operator*() const {
		auto* identity = reinterpret_cast<uint8_t*>(&_ukernel_identity_start);
		auto offset = reinterpret_cast<uint64_t>(m_ptr);

		return reinterpret_cast<T*>(identity+offset);
	}

	explicit operator bool(){
		return m_ptr != nullptr;
	}
};

class PhysAddr {
	void* m_ptr;
public:
	explicit PhysAddr(void* addr) noexcept
	: m_ptr(addr) {}

	template<class T>
	PhysPtr<T> as() {
		return PhysPtr<T>{reinterpret_cast<T*>(m_ptr)};
	}


};