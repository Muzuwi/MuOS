#include <LibGeneric/Memory.hpp>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>

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

	struct MyStructType {};

	size_t func_that_takes_struct(MyStructType) {
		return 0;
	}

	void perfect_forwarding_no_match_for_call_test() {
		//  Regression test for:
		//  - error: no match for call to '(KFunction<long unsigned int(tests::MyStructType)>) (tests::MyStructType&)'
		KFunction<size_t(MyStructType)> quux = func_that_takes_struct;
		MyStructType type {};
		auto v = quux(type);
		(void)v;
	}
}
