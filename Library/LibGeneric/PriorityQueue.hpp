#pragma once
#include <LibGeneric/Allocator.hpp>
#include <LibGeneric/Functional.hpp>
#include <LibGeneric/List.hpp>

namespace gen {
	template<class T, class Comparator = gen::less<T>, template<class> class Alloc = gen::Allocator>
	class PriorityQueue : protected gen::List<T, Alloc> {
	private:
		[[no_unique_address]] Comparator m_compare;
	public:
		PriorityQueue()
		    : gen::List<T, Alloc>() {}

		~PriorityQueue() = default;

		constexpr void push(T const& value) {
			auto it = gen::List<T, Alloc>::begin();
			const auto end = gen::List<T, Alloc>::end();
			while(it != end) {
				T const& rhs = *it;
				if(m_compare(value, rhs)) {
					break;
				}
				++it;
			}
			gen::List<T, Allocator>::insert(it, value);
		}

		constexpr void pop_back() { gen::List<T, Alloc>::pop_back(); }

		constexpr void pop_front() { gen::List<T, Alloc>::pop_front(); }

		constexpr bool empty() const { return gen::List<T, Alloc>::empty(); }

		constexpr void erase(auto it) { gen::List<T, Allocator>::erase(it); }

		constexpr T& front() { return gen::List<T, Alloc>::front(); }

		constexpr T const& front() const { return gen::List<T, Alloc>::front(); }

		constexpr T& back() { return gen::List<T, Alloc>::back(); }

		constexpr T const& back() const { return gen::List<T, Alloc>::back(); }

		constexpr auto begin() { return gen::List<T, Alloc>::begin(); }

		constexpr auto begin() const { return gen::List<T, Alloc>::begin(); }

		constexpr auto end() { return gen::List<T, Alloc>::end(); }

		constexpr auto end() const { return gen::List<T, Alloc>::end(); }
	};
}
