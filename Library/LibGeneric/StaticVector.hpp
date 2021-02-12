#pragma once
#ifdef __is_kernel_build__
#include <Kernel/Debug/kassert.hpp>
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
			return _get(m_pointer--);
		}
	public:
		StaticVector() noexcept
		: m_pointer(0) {}

		StaticVector(const StaticVector& v) noexcept {
			m_pointer = v.m_pointer;
			for(unsigned i = 0; i < m_pointer; ++i)
				m_buffer[i] = v.m_buffer[i];
		}

		size_t size() const {
			return m_pointer;
		}

		void push_back(T val) {
			_insert(val);
		}

		template<typename... Args>
		void emplace_back(Args&&... args) {
			assert(m_pointer < buffer_size);
			_get(m_pointer++) = T(args...);
		}

		T pop_back() {
			return _remove_back();
		}

		void clear() {
			m_pointer = 0;
		}

		bool empty() const {
			return m_pointer == 0;
		}

		T& operator[](size_t n) {
			return _get(n);
		}

	};

}