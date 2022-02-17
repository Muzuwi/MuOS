#pragma once
#include <LibGeneric/Iterator.hpp>

namespace gen {
	/*
	 *  Class representing a bidirectional iterator, that can be incremented and decremented.
	 */
	template<class T>
	class BidirectionalIterator : public Iterator<T> {
	public:
		BidirectionalIterator(const T&);

		BidirectionalIterator& operator++();
		BidirectionalIterator operator++(int);
		BidirectionalIterator& operator--();
		BidirectionalIterator operator--(int);

		bool operator==(const BidirectionalIterator& rhs) const;
		bool operator!=(const BidirectionalIterator& rhs) const;

		typename Iterator<T>::value_type operator*();
	};
}