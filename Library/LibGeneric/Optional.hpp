#pragma once
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Move.hpp>
#ifndef __is_kernel_build__
#	include <cassert>
#else
#	include <Debug/kassert.hpp>
#endif

namespace gen {
	struct __NullOpt {
		explicit constexpr __NullOpt(int) {};
	};

	inline constexpr __NullOpt nullopt { 1337 };

	template<class T>
	class Optional {
		enum class OptType {
			None,
			Some
		};

		OptType m_type;
		T m_data;
	public:
		/*
		 *  Constructs an Optional object containing no value.
		 */
		constexpr Optional() noexcept
		    : m_type(OptType::None) {}

		/*
		 *  Constructs an Optional object containing no value.
		 *  Explicit version of the default constructor.
		 */
		constexpr Optional(gen::__NullOpt) noexcept
		    : m_type(OptType::None) {}

		/*
		 *  Copies an Optional object.
		 */
		constexpr Optional(Optional const& other) noexcept {
			if(other.has_value()) {
				m_type = OptType::Some;
				gen::construct_at(&m_data, other.m_data);
			} else {
				m_type = OptType::None;
			}
		}

		/*
		 *  Constructs an Optional object from a different one, and moves its data
		 *  into the new one.
		 */
		constexpr Optional(Optional&& other) noexcept {
			if(other.has_value()) {
				m_type = OptType::Some;
				gen::construct_at(&m_data, gen::move(other.m_data));
			} else {
				m_type = OptType::None;
			}
		}

		/*
		 *  Copy conversion constructor
		 */
		template<class U>
		constexpr Optional(Optional<U> const& other) {
			if(other.has_value()) {
				m_type = OptType::Some;
				gen::construct_at(&m_data, other.m_data);
			} else {
				m_type = OptType::None;
			}
		}

		/*
		 *  Move conversion constructor
		 */
		template<class U>
		constexpr Optional(Optional<U>&& other) noexcept {
			if(other.has_value()) {
				m_type = OptType::Some;
				gen::construct_at(&m_data, gen::move(other.m_data));
			} else {
				m_type = OptType::None;
			}
		}

		/*
		 *  Constructs an Optional from a value convertible to T.
		 */
		template<class U = T>
		constexpr Optional(U value) noexcept
		    : m_type(OptType::Some)
		    , m_data(gen::move(value)) {}

		constexpr Optional& operator=(gen::__NullOpt) noexcept {
			if(has_value()) {
				reset();
			}
			return *this;
		}

		constexpr Optional& operator=(Optional const& other) noexcept {
			reset();
			if(!other.has_value()) {
				return *this;
			}
			m_type = other.m_type;
			gen::construct_at(&m_data, other.m_data);
			return *this;
		}

		constexpr Optional& operator=(Optional&& other) noexcept {
			reset();
			if(!other.has_value()) {
				return *this;
			}
			m_type = other.m_type;
			gen::construct_at(&m_data, gen::move(other.m_data));
			return *this;
		}

		template<class U = T>
		constexpr Optional& operator=(U value) {
			if(has_value()) {
				reset();
			}
			m_type = OptType::Some;
			gen::construct_at(&m_data, gen::move(value));
			return *this;
		}

		/*
		 *  Returns the status of the Optional - true if the Optional contains a value.
		 */
		constexpr bool has_value() const { return m_type == OptType::Some; }

		constexpr explicit operator bool() { return has_value(); }

		/*
		 *  Returns a reference to the stored data.
		 *  If no data is currently stored, triggers a panic.
		 */
		constexpr T& unwrap() {
			assert(has_value());
			return m_data;
		}

		/*
		 *  Returns a const reference to the stored data.
		 *  If no data is currently stored, triggers a panic.
		 */
		constexpr T const& unwrap() const {
			assert(has_value());
			return m_data;
		}

		/*
		 *  Returns the currently stored data, or a specified default value if no
		 *  value is currently stored.
		 */
		template<class U>
		constexpr T unwrap_or_default(U&& default_value) const {
			if(has_value()) {
				return m_data;
			} else {
				return static_cast<T>(gen::forward<U>(default_value));
			}
		}

		/*
		 *  Returns the currently stored data by moving it out of the Optional.
		 *  If no value is currently stored, a specified default value is returned instead.
		 */
		template<class U>
		constexpr T unwrap_or_default(U&& default_value) {
			if(has_value()) {
				return gen::move(m_data);
			} else {
				return static_cast<T>(gen::forward<U>(default_value));
			}
		}

		/*
		 *  Resets the Optional object and destroys the stored data if any is available.
		 */
		constexpr void reset() {
			if(!has_value()) {
				return;
			}

			m_type = OptType::None;
			gen::destroy_at(&m_data);
		}

		/*
		 *  Resets the Optional object, and constructs the newly stored value in-place.
		 */
		template<class... Args>
		constexpr T& emplace(Args&&... args) {
			if(has_value()) {
				reset();
			}

			m_type = OptType::Some;
			gen::construct_at(&m_data, gen::forward<Args>(args)...);
			return m_data;
		}
	};
}
