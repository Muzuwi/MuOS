#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Kernel/Symbols.hpp>
#include <Debug/klogf.hpp>
//#include <Kernel/Memory/PageToken.hpp>

template<class T>
class PhysPtr {
	T* m_ptr;

	static inline T* _to_identity_space(T* ptr) {
		auto* identity = reinterpret_cast<uint8_t*>(&_ukernel_identity_start);
		auto offset = reinterpret_cast<uint64_t>(ptr);

		return reinterpret_cast<T*>(identity+offset);
	}
public:
	PhysPtr() noexcept
	: m_ptr(nullptr) {}

	explicit PhysPtr(T* ptr) noexcept
	: m_ptr(ptr) {}

//	PhysPtr(const PageToken& token) noexcept
//	: m_ptr(reinterpret_cast<T*>(token.address())) {}   //  FIXME: I don't like this cast

	T* get() {
		return m_ptr;
	}

	const T* get() const {
		return m_ptr;
	}

	T* get_mapped() {
		return _to_identity_space(m_ptr);
	}

	T const* get_mapped() const {
		return _to_identity_space(m_ptr);
	}

	T& operator*() {
		return *_to_identity_space(m_ptr);
	}

	T const& operator*() const {
		return *_to_identity_space(m_ptr);
	}

	T& operator[](size_t index) {
		return *(_to_identity_space(m_ptr)+index);
	}

	T const& operator[](size_t index) const {
		return *(_to_identity_space(m_ptr)+index);
	}

	T* operator->() {
		return _to_identity_space(m_ptr);
	}

	T const* operator->() const{
		return _to_identity_space(m_ptr);
	}

	PhysPtr& operator++() {
		m_ptr++;
		return *this;
	}

	PhysPtr operator++(int) {
		auto temp = *this;
		operator++();
		return temp;
	}

	PhysPtr operator+(size_t offset) const {
		auto temp = *this;
		temp.m_ptr += offset;
		return temp;
	}

	PhysPtr operator-(size_t offset) const {
		auto temp = *this;
		temp.m_ptr -= offset;
		return temp;
	}

	size_t operator-(PhysPtr other_ptr) const {
		return m_ptr - other_ptr.m_ptr;
	}

	PhysPtr& operator=(T* ptr) {
		m_ptr = ptr;
		return *this;
	}

	explicit operator bool() const {
		return m_ptr != nullptr;
	}
};

class PhysAddr {
	void* m_ptr;
public:
	PhysAddr() noexcept
	: m_ptr(nullptr) {}

	explicit PhysAddr(void* addr) noexcept
	: m_ptr(addr) {
		if((uint64_t)addr & 0xffff000000000000) {
			kerrorf_static("Warning: PhysAddr constructed with a potentially virtual pointer [{}]\n", addr);
		}
	}

	template<class T>
	PhysPtr<T> as() {
		return PhysPtr<T>{reinterpret_cast<T*>(m_ptr)};
	}

	bool operator==(const PhysAddr& addr) const {
		return m_ptr == addr.m_ptr;
	}

	PhysAddr operator+(size_t offset) const {
		auto temp = *this;
		temp.operator+=(offset);
		return temp;
	}

	PhysAddr operator-(size_t offset) const {
		auto temp = *this;
		temp.operator-=(offset);
		return temp;
	}

	size_t operator-(PhysAddr v) const {
		return (uintptr_t)m_ptr - (uintptr_t)v.m_ptr;
	}

	PhysAddr& operator+=(size_t offset) {
		m_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ptr) + offset);
		return *this;
	}

	PhysAddr& operator-=(size_t offset) {
		m_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ptr) - offset);
		return *this;
	}

	bool operator>(const PhysAddr& v) const {
		return m_ptr > v.m_ptr;
	}

	bool operator<(const PhysAddr& v) const {
		return m_ptr < v.m_ptr;
	}

	bool operator<=(const PhysAddr& v) const {
		return m_ptr <= v.m_ptr;
	}

	bool operator>=(const PhysAddr& v) const {
		return m_ptr >= v.m_ptr;
	}

	void* get() {
		return m_ptr;
	}

	void* get_mapped() {
		return as<uint8_t>().get_mapped();
	}
};