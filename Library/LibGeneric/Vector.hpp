#pragma once
#include <LibGeneric/Allocator.hpp>
#include <LibGeneric/InitializerList.hpp>
#include <stddef.h>
#include "Platform/Assert.hpp"

namespace gen {
	template<class T, template<typename> class Alloc = gen::Allocator>
	class Vector {
		T* m_data;
		size_t m_buffer_size;
		size_t m_item_count;

		typedef typename Alloc<T>::template rebind<T>::other ObjAllocType;
		typename AllocatorTraits<ObjAllocType>::allocator_type _allocator;

		T* _buffer_allocate(size_t n) { return AllocatorTraits<ObjAllocType>::allocate(n); }

		void _buffer_free(T* base, size_t buffer_size) { AllocatorTraits<ObjAllocType>::deallocate(base, buffer_size); }

		void _object_destroy(T* buffer, size_t n) {
			if(buffer == nullptr) {
				return;
			}

			for(auto* p = buffer; p < buffer + n; ++p)
				AllocatorTraits<ObjAllocType>::destroy(_allocator, p);
		}

		void _object_construct(T* buffer, size_t n, const T& value) {
			if(buffer == nullptr) {
				return;
			}

			for(auto* p = buffer; p < buffer + n; ++p)
				AllocatorTraits<ObjAllocType>::construct(_allocator, p, value);
		}

		void _construct_from(T const* source, size_t count) {
			if(source == nullptr) {
				return;
			}

			for(auto* p = m_data; p < m_data + count && p < m_data + m_buffer_size; ++p) {
				AllocatorTraits<ObjAllocType>::construct(_allocator, p, *source);
				source++;
			}
		}

		void _deallocate_buffer() {
			_object_destroy(m_data, m_item_count);
			_buffer_free(m_data, m_buffer_size);
			m_data = nullptr;
			m_item_count = 0;
			m_buffer_size = 0;
		}

		void _resize_if_needed() {
			if(m_buffer_size != m_item_count)
				return;

			auto new_size = 2 * m_buffer_size;
			if(new_size == 0)
				new_size = 1;

			_buffer_reallocate(new_size);
		}

		void _buffer_reallocate(size_t new_size) {
			auto* old_buf = m_data;
			auto old_size = m_buffer_size;
			m_data = _buffer_allocate(new_size);
			m_buffer_size = new_size;
			_construct_from(old_buf, m_item_count);
			_object_destroy(old_buf, m_item_count);
			_buffer_free(old_buf, old_size);
		}

		void from_vector(Vector const& v) {
			if(v.m_item_count == 0) {
				m_data = nullptr;
				m_buffer_size = 0;
				m_item_count = 0;
			} else {
				m_data = _buffer_allocate(v.m_item_count);
				m_buffer_size = v.m_item_count;
				_construct_from(v.m_data, v.m_item_count);
				m_item_count = v.m_item_count;
			}
		}
	public:
		constexpr Vector() noexcept
		    : m_data(nullptr)
		    , m_buffer_size(0)
		    , m_item_count(0) {}

		constexpr Vector(Vector const& v) noexcept { from_vector(v); }

		constexpr Vector(Vector&& v) noexcept
		    : m_data(v.m_data)
		    , m_buffer_size(v.m_buffer_size)
		    , m_item_count(v.m_item_count) {}

		constexpr ~Vector() { _deallocate_buffer(); }

		void push_back(const T& v) {
			_resize_if_needed();
			_object_construct(m_data + m_item_count, 1, v);
			m_item_count++;
		}

		T pop_back() {
			LIBGEN_ASSERT(m_item_count > 0);

			auto v = m_data[--m_item_count];
			_object_destroy(m_data + m_item_count, 1);

			return v;
		}

		void clear() { _deallocate_buffer(); }

		void resize(size_t n, const T& val = T()) {
			if(n < m_item_count) {
				//  Reduce content to first n elements
				_object_destroy(m_data + n, m_item_count - n);
			} else {
				//  Reallocate when needed
				if(n > m_buffer_size)
					_buffer_reallocate(n);
				//  Construct new elements
				_object_construct(m_data + m_item_count, n - m_item_count, val);
			}

			m_item_count = n;
		}

		void reserve(size_t n) {
			if(n > m_buffer_size)
				_buffer_reallocate(n);
		}

		constexpr T& operator[](size_t n) { return m_data[n]; }

		constexpr T const& operator[](size_t n) const { return m_data[n]; }

		T& at(size_t n) {
			LIBGEN_ASSERT(n < m_item_count);
			return operator[](n);
		}

		T const& at(size_t n) const {
			LIBGEN_ASSERT(n < m_item_count);
			return operator[](n);
		}

		size_t size() const { return m_item_count; }

		size_t capacity() const { return m_buffer_size; }

		bool empty() const { return size() == 0; }

		using iterator = T*;
		using const_iterator = T const*;

		iterator begin() { return &m_data[0]; }

		const_iterator begin() const { return &m_data[0]; }

		iterator end() { return &m_data[m_item_count]; }

		const_iterator end() const { return &m_data[m_item_count]; }

		Vector& operator=(Vector const& v) {
			if(&v == this) {
				return *this;
			}
			from_vector(v);
			return *this;
		}
	};
}