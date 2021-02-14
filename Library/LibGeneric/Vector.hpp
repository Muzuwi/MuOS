#pragma once
#include <stddef.h>
#include <LibGeneric/InitializerList.hpp>
#include <LibGeneric/Allocator.hpp>

#ifdef __is_kernel_build__
#include <Kernel/Debug/kassert.hpp>
#endif

namespace gen {
	template<class T, template<typename> class Alloc = gen::Allocator>
	class Vector {
		T* m_data;
		size_t m_buffer_size;
		size_t m_item_count;

		typedef typename Alloc<T>::template rebind<T>::other ObjAllocType;
		typename AllocatorTraits<ObjAllocType>::allocator_type _allocator;

		T* _buffer_allocate(size_t n) {
			return AllocatorTraits<ObjAllocType>::allocate(n);
		}

		void _buffer_free(T* base, size_t buffer_size) {
			AllocatorTraits<ObjAllocType>::deallocate(base, buffer_size);
		}

//		void _buffer_delete(T* base, size_t constr_count, size_t obj_count) {
//			for(auto* p = base; p < base + constr_count; ++p)
//				AllocatorTraits<ObjAllocType>::destroy(_allocator, p);
//			AllocatorTraits<ObjAllocType>::deallocate(base, obj_count);
//		}

//		T* _buffer_create(size_t n) {
//			auto* buf = AllocatorTraits<ObjAllocType>::allocate(n);
//			for(auto* p = buf; p < buf + n; ++p)
//				AllocatorTraits<ObjAllocType>::construct(_allocator, p);
//			return buf;
//		}

		void _object_destroy(T* buffer, size_t n) {
			for(auto* p = buffer; p < buffer + n; ++p)
				AllocatorTraits<ObjAllocType>::destroy(_allocator, p);
		}

		void _object_construct(T* buffer, size_t n, const T& value) {
			for(auto* p = buffer; p < buffer + n; ++p)
				AllocatorTraits<ObjAllocType>::construct(_allocator, p, value);
		}

		void _construct_from(T const* source, size_t count) {
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
	public:
		Vector() noexcept
		: m_data(nullptr), m_buffer_size(0), m_item_count(0) {}

		Vector(const Vector& v) noexcept {
			m_data = _buffer_allocate(v.m_item_count);
			m_buffer_size = v.m_item_count;
			_construct_from(v.m_data, v.m_item_count);
			m_item_count = v.m_item_count;
		}

		Vector(Vector&& v) noexcept
		: m_data(v.m_data), m_buffer_size(v.m_buffer_size), m_item_count(v.m_item_count) {}

		Vector(std::initializer_list<T> initializer) noexcept = delete; //  TODO: For fun

		~Vector() {
			_deallocate_buffer();
		}

		void push_back(const T& v) {
			_resize_if_needed();
			_object_construct(m_data+m_item_count, 1, v);
			m_item_count++;
		}

		T pop_back() {
			assert(m_item_count > 0);

			auto v = m_data[--m_item_count];
			_object_destroy(m_data + m_item_count, 1);

			return v;
		}

		void clear() {
			_deallocate_buffer();
		}

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


		T& operator[](size_t n) {
			return m_data[n];
		}

		T const& operator[](size_t n) const {
			return m_data[n];
		}

		T& at(size_t n) {
			assert(n < m_item_count);
			return operator[](n);
		}

		T const& at(size_t n) const {
			assert(n < m_item_count);
			return operator[](n);
		}


		size_t size() const {
			return m_item_count;
		}

		size_t capacity() const {
			return m_buffer_size;
		}

		bool empty() const {
			return size() == 0;
		}

		using iterator = T*;
		using const_iterator = T const*;

		iterator begin() {
			return &m_data[0];
		}

		const_iterator begin() const {
			return &m_data[0];
		}

		iterator end() {
			return &m_data[m_item_count];
		}

		const_iterator end() const {
			return &m_data[m_item_count];
		}

	};
}