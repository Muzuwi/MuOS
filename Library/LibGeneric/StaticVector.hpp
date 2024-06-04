#pragma once
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Move.hpp>
#include <stddef.h>
#include <stdint.h>
#ifdef __is_kernel_build__
#	include <Core/Assert/Assert.hpp>
#endif

namespace gen {
	/*
	 *  A vector-like container that encapsulates dynamic item addition and removal.
	 *  Instead of allocating, a buffer of pre-declared size is used.
	 */
	template<typename T, size_t bufsize>
	class StaticVector {
		T m_data[bufsize];
		size_t m_pointer;

		template<typename... Args>
		constexpr void construct_at(size_t n, Args&&... args) {
			gen::construct_at(m_data + n, gen::forward<Args>(args)...);
		}

		constexpr void destroy_at(size_t n) { gen::destroy_at(m_data + n); }

		constexpr void destroy_all() {
			for(auto i = 0u; i < m_pointer; ++i) {
				destroy_at(i);
			}
		}
	public:
		constexpr StaticVector() noexcept
		    : m_pointer(0) {}

		/*
		 *  Destroys all stored elements in the vector
		 */
		constexpr ~StaticVector() { destroy_all(); }

		/*
		 *  Adds an item at the end of the vector. Returns true when the operation
		 *  succeeded, otherwise false.
		 *  The elements are constructed as if directly initialized.
		 */
		template<typename U = T>
		constexpr bool push_back(U&& value) {
			if(m_pointer >= bufsize) {
				return false;
			}

			construct_at(m_pointer++, gen::forward<U&&>(value));
			return true;
		}

		/*
		 *  Removes an element from the end of the vector.
		 */
		constexpr void pop_back() {
			if(!m_pointer) {
				return;
			}

			destroy_at(--m_pointer);
		}

		/*
		 *  Removes all elements from the vector.
		 */
		constexpr void clear() {
			destroy_all();
			m_pointer = 0;
		}

		/*
		 *  Amount of items currently stored in the vector.
		 */
		constexpr size_t size() const { return m_pointer; }

		constexpr bool empty() const { return m_pointer == 0; }

		/*
		 *  Total buffer capacity of the structure.
		 */
		constexpr size_t capacity() const { return bufsize; }

		using iterator = T*;
		using const_iterator = T const*;

		/*
		 *  Iterators
		 */
		constexpr iterator begin() { return m_data; }

		constexpr iterator end() { return m_data + m_pointer; }

		constexpr const_iterator begin() const { return m_data; }

		constexpr const_iterator end() const { return m_data + m_pointer; }

		/*
		 *  Array-index operator
		 */
		constexpr T& operator[](size_t n) {
			ENSURE(n < m_pointer);
			return m_data[n];
		}

		constexpr T const& operator[](size_t n) const {
			ENSURE(n < m_pointer);
			return m_data[n];
		}

		constexpr T& front() {
			ENSURE(m_pointer);
			return m_data[0];
		}

		constexpr T const& front() const {
			ENSURE(m_pointer);
			return m_data[0];
		}

		constexpr T& back() {
			ENSURE(m_pointer);
			return m_data[m_pointer - 1];
		}

		constexpr T const& back() const {
			ENSURE(m_pointer);
			return m_data[m_pointer - 1];
		}
	};
}