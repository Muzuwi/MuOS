#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __is_kernel_build__
#include <Debug/kassert.hpp>
#endif

namespace gen {
	template<class T, unsigned buffer_size>
	class StaticVector {
		static_assert(buffer_size > 0, "Invalid buffer size");
		T m_buffer[buffer_size];
		size_t m_pointer;

		T const& _get(size_t n) const {
			assert(n < buffer_size);
			return m_buffer[n];
		}

		T& _get(size_t n) {
			assert(n < buffer_size);
			return m_buffer[n];
		}

		void _insert(T val) {
			assert(m_pointer < buffer_size);
			m_buffer[m_pointer++] = val;
		}

		T _remove_back() {
			assert(!empty());
			return _get(--m_pointer);
		}

	public:
		StaticVector() noexcept
				: m_pointer(0) {}

		StaticVector(StaticVector const& v) noexcept {
			m_pointer = v.m_pointer;
			for(unsigned i = 0; i < m_pointer; ++i) {
				m_buffer[i] = v.m_buffer[i];
			}
		}

		size_t size() const {
			return m_pointer;
		}

		bool empty() const {
			return m_pointer == 0;
		}

		/*
		 *  Vec mutators
		 */
		void push_back(T val) {
			_insert(val);
		}

		T pop_back() {
			return _remove_back();
		}

		template<typename... Args>
		void emplace_back(Args&& ... args) {
			assert(m_pointer < buffer_size);
			_get(m_pointer++) = T(args...);
		}

		void clear() {
			m_pointer = 0;
		}

		T& operator[](size_t n) {
			return _get(n);
		}

		T const& operator[](size_t n) const {
			return _get(n);
		}

		using iterator = T*;
		using const_iterator = T const*;

		iterator begin() {
			return m_buffer;
		}

		const_iterator begin() const {
			return m_buffer;
		}

		iterator end() {
			return m_buffer + buffer_size;
		}

		const_iterator end() const {
			return m_buffer + buffer_size;
		}

	};

}