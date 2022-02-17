#pragma once
#include <Debug/kassert.hpp>

//  FIXME:  Hacky hack

template<class T>
class KOptional {
	T m_value;
	bool m_has_value;

	void _construct_none() { m_has_value = false; }

	void _construct_some(T value) {
		m_has_value = true;
		m_value = value;
	}
public:
	KOptional() { _construct_none(); }

	KOptional(T val) { _construct_some(val); }

	T unwrap() {
		assert(m_has_value);
		return m_value;
	}

	bool has_value() { return m_has_value; }
};
