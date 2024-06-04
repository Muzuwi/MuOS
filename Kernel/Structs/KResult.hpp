#pragma once
#include <Core/Assert/Assert.hpp>
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Optional.hpp>

/**	Object representing a value of T or ErrType.
 */
template<class T, class ErrType>
class KResult {
public:
	/**	Construct a KResult containing data
	 */
	constexpr explicit KResult(T data) noexcept
	    : m_result(Result::Value) {
		gen::construct_at(&m_data._data, gen::move(data));
	}

	/**	Construct a KResult containing an error
	 */
	constexpr explicit KResult(ErrType err_type) noexcept
	    : m_result(Result::Error) {
		gen::construct_at(&m_data._error, gen::move(err_type));
	}

	constexpr ~KResult() {
		if(m_result == Result::Error) {
			gen::destroy_at(&m_data._error);
		} else {
			gen::destroy_at(&m_data._data);
		}
	}

	/**	Return the contained data by moving it out of the KResult.
	 *	This performs a safety check at runtime. If the KResult does not contain
	 * 	a value, a kernel panic is triggered.
	 * 	WARNING: The KResult is left in an unspecified state. Calling any methods on
	 * 	the KResult after the move results in undefined behavior.
	 */
	constexpr T destructively_move_data() {
		ENSURE(m_result == Result::Value);
		//  TODO: This could store the moved state to verify the object is not
		//		  moved from at runtime.
		return gen::move(m_data._data);
	}

	/**	Return a copy of the contained data in the KResult.
	 *	This performs a safety check at runtime. If the KResult does not contain
	 * 	a value, a kernel panic is triggered.
	 */
	constexpr T data() const {
		ENSURE(m_result == Result::Value);
		return m_data._data;
	}

	/**	Return a copy of the error contained in the KResult.
	 *	This performs a safety check at runtime. If the KResult does not contain
	 * 	an error, a kernel panic is triggered.
	 */
	constexpr ErrType error() const {
		ENSURE(m_result == Result::Error);
		return m_data._error;
	}

	/**	Check if the KResult contains an error.
	 */
	[[nodiscard]] constexpr bool has_error() const { return m_result == Result::Error; }

	/**	Check if the KResult contains a value.
	 */
	[[nodiscard]] constexpr bool has_value() const { return m_result == Result::Value; }

	/**	Implicit bool conversion, equivalent to has_value()
	 */
	constexpr operator bool() const { return has_value(); }
private:
	enum class Result {
		Value,
		Error,
	};

	union {
		T _data;
		ErrType _error;
	} m_data;

	Result m_result;
};