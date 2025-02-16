#pragma once

namespace gen {
	/*
	 *  Iterator base class
	 */
	template<class T, class Pointer = T*, class Reference = T&>
	class Iterator {
	public:
		typedef T value_type;
		typedef Pointer ptr_type;
		typedef Reference ref_type;
	};
}