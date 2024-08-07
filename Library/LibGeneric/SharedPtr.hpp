#pragma once
#include <LibGeneric/Allocator.hpp>
#include <stddef.h>
#include <stdint.h>

namespace gen {
	template<template<typename> class Alloc = gen::Allocator>
	class __SharedCount {
	public:
		typedef size_t _refcount_t;
	private:
		_refcount_t* m_ref_count;

		typedef typename Alloc<_refcount_t>::template rebind<_refcount_t>::other RefAllocType;
		typename AllocatorTraits<RefAllocType>::allocator_type _ref_allocator;

		constexpr void _storage_free() {
			AllocatorTraits<RefAllocType>::destroy(_ref_allocator, m_ref_count);
			AllocatorTraits<RefAllocType>::deallocate(m_ref_count, 1);
			m_ref_count = nullptr;
		}

		constexpr void _add_ref() {
			if(!m_ref_count)
				return;

			++(*m_ref_count);
		}

		constexpr void _release_ref() {
			if(!m_ref_count)
				return;

			--(*m_ref_count);

			if(*m_ref_count == 0)
				_storage_free();
		}
	public:
		constexpr __SharedCount(_refcount_t* p) noexcept
		    : m_ref_count(p) {}

		__SharedCount(const __SharedCount& ptr) noexcept
		    : m_ref_count(ptr.m_ref_count) {
			_add_ref();
		}

		__SharedCount& operator=(const __SharedCount& c) {
			if(&c == this)
				return *this;

			_release_ref();
			m_ref_count = c.m_ref_count;
			_add_ref();

			return *this;
		}

		~__SharedCount() noexcept { _release_ref(); }

		_refcount_t _use_count() const { return ((m_ref_count) ? (*m_ref_count) : 0); }

		void _swap(__SharedCount& v) {
			auto* tmp = v.m_ref_count;
			v.m_ref_count = m_ref_count;
			m_ref_count = tmp;
		}
	};

	template<class T, template<typename> class Alloc = gen::Allocator>
	class SharedPtr {
		using RefCount = __SharedCount<Alloc>;

		T* m_ptr;
		RefCount m_ref_count;

		typedef typename Alloc<typename RefCount::_refcount_t>::template rebind<typename RefCount::_refcount_t>::other
		        RefAllocType;
		typename AllocatorTraits<RefAllocType>::allocator_type _ref_allocator;
		typedef typename Alloc<T>::template rebind<T>::other ObjAllocType;
		typename AllocatorTraits<ObjAllocType>::allocator_type _obj_allocator;

		constexpr void _on_destruct() {
			if(m_ref_count._use_count() == 1) {
				AllocatorTraits<ObjAllocType>::destroy(_obj_allocator, m_ptr);
				AllocatorTraits<ObjAllocType>::deallocate(m_ptr, 1);
				m_ptr = nullptr;
			}
		}

		inline typename RefCount::_refcount_t* _alloc_refcount() {
			auto* ptr = AllocatorTraits<RefAllocType>::allocate(1);
			AllocatorTraits<RefAllocType>::construct(_ref_allocator, ptr, 1);
			return ptr;
		}
	public:
		constexpr SharedPtr() noexcept
		    : m_ptr(nullptr)
		    , m_ref_count(nullptr) {}

		explicit constexpr SharedPtr(decltype(nullptr))
		    : SharedPtr() {}

		explicit SharedPtr(T* ptr) noexcept
		    : m_ptr(ptr)
		    , m_ref_count(_alloc_refcount()) {}

		SharedPtr(const SharedPtr& ptr) noexcept
		    : m_ptr(ptr.m_ptr)
		    , m_ref_count(ptr.m_ref_count) {}

		~SharedPtr() {
			//  Free when we're the only reference left
			_on_destruct();
		}

		SharedPtr& operator=(const SharedPtr& ptr) noexcept {
			if(&ptr == this)
				return *this;

			_on_destruct();
			m_ptr = ptr.m_ptr;
			m_ref_count = ptr.m_ref_count;

			return *this;
		}

		size_t use_count() const { return m_ref_count._use_count(); }

		T* get() const noexcept { return m_ptr; }

		T& operator*() const noexcept { return *m_ptr; }

		T* operator->() const noexcept { return m_ptr; }

		operator bool() const { return (m_ptr != nullptr); }

		void reset() noexcept {
			_on_destruct();
			m_ptr = nullptr;
			m_ref_count = nullptr;
		}

		template<class Y>
		void reset(Y* ptr) {
			_on_destruct();
			m_ptr = ptr;
			m_ref_count = _alloc_refcount();
		}

		void swap(SharedPtr& r) noexcept {
			auto* tmp = r.m_ptr;
			r.m_ptr = m_ptr;
			m_ptr = tmp;

			m_ref_count._swap(r.m_ref_count);
		}
	};

	template<class T, class... Args>
	SharedPtr<T> make_shared(Args&&... args) {
		return SharedPtr<T> { new (gen::Allocator<T>::allocate(1)) T(args...) };
	}
}
