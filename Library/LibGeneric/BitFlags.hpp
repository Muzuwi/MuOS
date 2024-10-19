#pragma once

/*  Define common bit flag operations for a given enumeration type
 *
 *  This defines overloads for common operations that can be done on a
 *  scoped enumeration type that contains enumerations for bit flags.
 * 	The support assumes an unsigned integer type as the backing type
 * 	for the enum (any size), signed types are NOT supported.
 *  This allows for:
 *
 *    - simple concatenation of flags without explicit static_cast, e.g
 *
 *          auto flags = type::Foo | type::Bar;
 *
 *    - masking flags with &, where the left-hand side of the expression
 * 		can be either an unsigned integer type or the enumeration type
 * 		itself. Do note that the result of the expression is widened to
 * 		a uintptr_t, so masking an enum with a uint32_t backing type will
 * 		result in a uintptr_t result.
 * 		As a result, checking for the presence of a flag is as simple as:
 *
 * 			uintptr_t v = ...;
 * 			if(v & type::Foo)
 *
 * 		or:
 *
 * 			type v = ...;
 * 			if(v & type::Foo)
 *
 */
#define DEFINE_ENUM_BITFLAG_OPS(type)                                                        \
	constexpr type operator|(type lhs, type rhs) {                                           \
		return static_cast<type>(static_cast<uintptr_t>(lhs) | static_cast<uintptr_t>(rhs)); \
	}                                                                                        \
	constexpr uintptr_t operator&(type lhs, type rhs) {                                      \
		return static_cast<uintptr_t>(lhs) & static_cast<uintptr_t>(rhs);                    \
	}                                                                                        \
	constexpr uintptr_t operator&(uintptr_t v, type f) {                                     \
		return v & static_cast<uintptr_t>(f);                                                \
	}
