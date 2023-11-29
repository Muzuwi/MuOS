#pragma once

#include "LibGeneric/Memory.hpp"
#include "LibGeneric/Optional.hpp"
#include "Structs/KOptional.hpp"
template<class T, class ErrType>
class KResult {
private:
	enum class Result {
		Value,
		Error
	};

	union {
		T _data;
		ErrType _error;
	} m_data;

	Result m_result;
public:
	constexpr explicit KResult(T data) noexcept
	    : m_result(Result::Value) {
		gen::construct_at(&m_data._data, data);
	}

	constexpr explicit KResult(ErrType err_type) noexcept
	    : m_result(Result::Error) {
		gen::construct_at(&m_data._error, err_type);
	}

	constexpr ~KResult() {
		if(m_result == Result::Error) {
			gen::destroy_at(&m_data._error);
		} else {
			gen::destroy_at(&m_data._data);
		}
	}

	T& unwrap() { return m_data._data; }

	T const& unwrap() const { return m_data._data; }

	KOptional<T> move() {
		if(m_result == Result::Error) {
			return { gen::nullopt };
		}
		return KOptional<T>(m_data._data);
	}

	ErrType const& error() const { return m_data._error; }

	bool has_error() const { return m_result == Result::Error; }
	bool has_value() const { return m_result == Result::Value; }
};