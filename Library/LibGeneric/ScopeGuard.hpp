#pragma once
#include <LibGeneric/Move.hpp>

namespace gen {
	template<typename Callable>
	class ScopeGuard {
	public:
		constexpr ScopeGuard(Callable callable)
		    : m_callable(gen::move(callable)) {}

		~ScopeGuard() { m_callable(); }
	private:
		Callable m_callable;
	};
}

#define __MCONCAT_NONEXPANDED(A, B) A##B
#define __MCONCAT(A, B)             __MCONCAT_NONEXPANDED(A, B)

/** Clean up resources when this scope exits.
 *  WARNING: All variables in the current scope are reference-captured.
 *
 * 	Example usage:
 *
 * 		DEFER {
 *			// perform some cleanup at scope exit..
 * 		};
 */
#define DEFER gen::ScopeGuard __MCONCAT(__scope_guard_, __LINE__) = [&]
