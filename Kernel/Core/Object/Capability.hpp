#pragma once
#include <LibGeneric/String.hpp>

namespace core::object {
	template<typename T>
	concept Capability = requires(T value) {
		                     { T::type() };
	                     };
}
