#pragma once

#include <stdint.h>
#include <stddef.h>

namespace std {
    template<class _E>
    class initializer_list {
	public:
		typedef _E value_type;
		typedef const _E& reference;
		typedef const _E& const_reference;
		typedef size_t size_type;
		typedef const _E* iterator;
		typedef const _E* const_iterator;
	private:
		iterator m_array;
		size_type m_len;

		constexpr initializer_list(const_iterator array, size_type list)
		    : m_array(array), m_len(list) { }

	public:
		constexpr initializer_list() noexcept
		    : m_array(nullptr), m_len(0) { }

		constexpr size_type size() const noexcept {
			return m_len;
		}

		constexpr const_iterator begin() const noexcept {
			return m_array;
		}

		constexpr const_iterator end() const noexcept {
			return begin() + size();
		}
	};

	template<class _Tp>
	constexpr const _Tp* begin(initializer_list<_Tp> list) noexcept {
		return list.begin();
	}

	template<class _Tp>
	constexpr const _Tp* end(initializer_list<_Tp> list) noexcept {
		return list.end();
	}


}
