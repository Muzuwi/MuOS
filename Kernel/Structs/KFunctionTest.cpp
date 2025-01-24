#include <LibGeneric/Memory.hpp>
#include <Structs/KFunction.hpp>

namespace tests {
	void _empty() {}

	//  Check if basic KFunction usage builds
	void compile_test() {
		//  Simple construction
		KFunction<void()> foo = []() {};
		KFunction<void()> baz = _empty;
		//  Copy construction
		KFunction<void()> bar;
		bar = foo;
		//  invoke()
		bar.invoke();
	}
}
