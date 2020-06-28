#pragma once
#include <LibGeneric/Iterator.hpp>

namespace gen {
	template<class T>
	constexpr const T& min(const T& a, const T& b) {
		return !(b<a) ? a : b;
	}

	template<class T, class Compare>
	constexpr const T& min(const T& a, const T& b, Compare comp) {
		return !comp(b,a) ? a : b;
	}

	template<class T>
	constexpr const T& max(const T& a, const T& b) {
		return (a<b) ? b : a;
	}

	template<class T, class Compare>
	constexpr const T& max(const T& a, const T& b, Compare comp) {
		return comp(a,b) ? b : a;
	}

	template<class InputIterator, class V>
	constexpr InputIterator find(InputIterator begin, InputIterator end, V val) {
		InputIterator it = begin;
		while(it != end) {
			if(*it == val) return it;
			++it;
		}
		return it;
	}

	template<class Container, class V>
	constexpr auto find(const Container& container, const V& val) {
		auto it = container.begin();
		while(it != container.end()) {
			if(*it == val) return it;
			++it;
		}
		return it;
	}

	template<class Container, class V>
	constexpr auto find(Container& container, const V& val) {
		auto it = container.begin();
		while(it != container.end()) {
			if(*it == val) return it;
			++it;
		}
		return it;
	}


	template<class InputIterator, class UnaryPredicate>
	constexpr InputIterator find_if(InputIterator begin, InputIterator end, UnaryPredicate pred) {
		InputIterator it = begin;
		while(it != end) {
			if(pred(*it)) return it;
			++it;
		}
		return it;
	}

	template<class Container, class UnaryPredicate>
	constexpr auto find_if(const Container& container, UnaryPredicate pred) {
		auto it = container.begin();
		while(it != container.end()) {
			if(pred(*it)) return it;
			++it;
		}
		return it;
	}

	template<class Container, class UnaryPredicate>
	constexpr auto find_if(Container& container, UnaryPredicate pred) {
		auto it = container.begin();
		while(it != container.end()) {
			if(pred(*it)) return it;
			++it;
		}
		return it;
	}

	template<class T>
	constexpr void swap(T& a, T& b) {
		T c (a);
		a = b;
		b = c;
	}
}