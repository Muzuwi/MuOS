#pragma once
#include <Kernel/Memory/PageToken.hpp>

template<class T>
class PhysPtr {
	T* m_ptr;
public:
	explicit PhysPtr(T* ptr)
	: m_ptr(ptr) {}

	PhysPtr(const PageToken& token)
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

	explicit operator bool(){
		return m_ptr != nullptr;
	}
};