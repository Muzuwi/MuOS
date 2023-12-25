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

/** Clean up resources when this scope exits.
 *  WARNING: All variables in the current scope are reference-captured.
 */
#define DEFER gen::ScopeGuard foo = [&]

inline void foo() {
	DEFER {
		//  cleanup
	};
}
