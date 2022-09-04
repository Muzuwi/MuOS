#include <catch2/catch.hpp>
#include <LibGeneric/Function.hpp>

TEST_CASE("gen::Function", "[structs]") {
	SECTION("no captures") {
		gen::Function<int()> func1 = []() -> int { return 1; };
		REQUIRE(func1() == 1);
	}

	SECTION("capture-by-value") {
		int test = 1337;
		gen::Function<int()> func2 = [test]() -> int { return test; };
		REQUIRE(func2() == 1337);

		struct DestructionProbe {
			int* ptr;

			constexpr DestructionProbe(int* p)
			    : ptr(p) {}

			constexpr int foo() const { return 1337; }

			constexpr ~DestructionProbe() { *ptr += 1; }
		};

		SECTION("destructors were called") {
			int counter = 0;
			{
				DestructionProbe p { &counter };
				gen::Function<int()> func { [p]() -> int { return p.foo(); } };
				REQUIRE(func() == 1337);
			}
			REQUIRE(counter == 3);
		}
	}

	SECTION("function parameters") {
		gen::Function<int(int)> func3 = [](int a) -> int { return a + 1; };
		REQUIRE(func3(10) == 11);
	}

	SECTION("capture-by-reference") {
		int counter = 1;
		int test = 1337;
		gen::Function<int(int)> func4 = [&](int a) -> int {
			counter += 1;
			return a + 1 + test;
		};
		REQUIRE(func4(10) == 1348);
		REQUIRE(counter == 2);
	}
}
