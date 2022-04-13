#pragma once
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Move.hpp>
#ifndef __is_kernel_build__
#	include <cassert>
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
		constexpr Optional() noexcept
		    : m_type(OptType::None) {}

		constexpr Optional(gen::__NullOpt) noexcept
		    : m_type(OptType::None) {}

		constexpr Optional(Optional const& other) noexcept {
			if(other.has_value()) {
				m_type = OptType::Some;
				gen::construct_at(&m_data, other.m_data);
			} else {
				m_type = OptType::None;
			}
		}

		constexpr Optional(Optional&& other) noexcept = delete;

		template<class U>
		constexpr Optional(Optional<U> const& other) {
			if(other.has_value()) {
				m_type = OptType::Some;
				gen::construct_at(&m_data, other.m_data);
			} else {
				m_type = OptType::None;
			}
		}

		template<class U>
		constexpr Optional(Optional<U>&& other) noexcept = delete;

		template<class U = T>
		constexpr Optional(U&& value) noexcept
		    : m_type(OptType::Some)
		    , m_data(gen::forward<U>(value)) {}

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

		constexpr Optional& operator=(Optional&& other) noexcept = delete;

		template<class U = T>
		constexpr Optional& operator=(U&& value) {
			if(has_value()) {
				reset();
			}
			m_type = OptType::Some;
			gen::construct_at(&m_data, gen::forward<U>(value));
			return *this;
		}

		constexpr bool has_value() const { return m_type == OptType::Some; }

		constexpr explicit operator bool() { return has_value(); }

		constexpr T& unwrap() {
			assert(has_value());
			return m_data;
		}

		constexpr T const& unwrap() const {
			assert(has_value());
			return m_data;
		}

		template<class U>
		constexpr T unwrap_or_default(U&& default_value) const {
			if(has_value()) {
				return m_data;
			} else {
				return static_cast<T>(gen::forward<U>(default_value));
			}
		}

		template<class U>
		constexpr T unwrap_or_default(U&& default_value) {
			if(has_value()) {
				return gen::move(m_data);
			} else {
				return static_cast<T>(gen::forward<U>(default_value));
			}
		}

		constexpr void reset() {
			if(!has_value()) {
				return;
			}

			m_type = OptType::None;
			gen::destroy_at(&m_data);
		}

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
