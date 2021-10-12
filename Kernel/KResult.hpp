#pragma once

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
	explicit KResult(const T& data) noexcept
	: m_result(Result::Value) {
		m_data._data = data;
	}

	explicit KResult(const ErrType& err_type) noexcept
	: m_result(Result::Error) {
		m_data._error = err_type;
	}

	T& unwrap() {
		return m_data._data;
	}

	T const& unwrap() const {
		return m_data._data;
	}

	ErrType const& error() const {
		return m_data._error;
	}

	bool has_error() const { return m_result == Result::Error; }
	bool has_value() const { return m_result == Result::Value; }
};