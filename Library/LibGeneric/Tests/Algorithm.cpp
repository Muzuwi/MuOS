#include <array>
#include <iterator>
#include <LibGeneric/Algorithm.hpp>
#include <catch2/catch.hpp>

TEST_CASE("gen::find", "[algo]") {
	SECTION("find with begin-end by value") {
		std::array<int, 5> arr { 111, 222, 333, 444, 555 };

		auto it = gen::find(arr.begin(), arr.end(), 333);
		REQUIRE(it != arr.end());
		auto target = arr.begin();
		std::advance(target, 2);
		REQUIRE(it == target);
	}

	SECTION("find with container by value") {
		std::array<int, 5> arr { 111, 222, 333, 444, 555 };

		auto it = gen::find(arr, 333);
		REQUIRE(it != arr.end());
		auto target = arr.begin();
		std::advance(target, 2);
		REQUIRE(it == target);
	}

	SECTION("find_if with begin-end") {
		std::array<int, 5> arr { 111, 222, 333, 444, 555 };

		auto it = gen::find_if(arr.begin(), arr.end(), [](int& value) {
			return value == 444;
		});
		REQUIRE(it != arr.end());
		auto target = arr.begin();
		std::advance(target, 3);
		REQUIRE(it == target);
	}

	SECTION("find_if with container") {
		std::array<int, 5> arr { 111, 222, 333, 444, 555 };

		auto it = gen::find_if(arr, [](int& value) {
			return value == 444;
		});
		REQUIRE(it != arr.end());
		auto target = arr.begin();
		std::advance(target, 3);
		REQUIRE(it == target);
	}
}

TEST_CASE("gen::min", "[algo]") {
	SECTION("a < b") {
		REQUIRE(gen::min(-1, 1) == -1);
	}

	SECTION("b < a") {
		REQUIRE(gen::min(1, -1) == -1);
	}

	SECTION("custom comparator") {
		struct MyStruct {
			int value;
		};

		MyStruct foo { .value = -1 };
		MyStruct bar { .value = -20 };

		auto const& smaller = gen::min(foo, bar, [](MyStruct const& lhs, MyStruct const& rhs) {
			return lhs.value < rhs.value;
		});
		REQUIRE(&smaller == &bar);
	}
}

TEST_CASE("gen::max", "[algo]") {
	SECTION("a > b") {
		REQUIRE(gen::max(100, 1) == 100);
	}

	SECTION("b > a") {
		REQUIRE(gen::max(100, 1) == 100);
	}

	SECTION("custom comparator") {
		struct MyStruct {
			int value;
		};

		MyStruct foo { .value = -1 };
		MyStruct bar { .value = -20 };

		auto const& smaller = gen::min(foo, bar, [](MyStruct const& lhs, MyStruct const& rhs) {
			return lhs.value >= rhs.value;
		});
		REQUIRE(&smaller == &foo);
	}
}

TEST_CASE("gen::swap", "[algo]") {
	SECTION("primitive swap") {
		int a = 5, b = 3;
		gen::swap(a, b);
		REQUIRE(a == 3);
		REQUIRE(b == 5);
	}

	SECTION("non-primitive swap") {
		struct MyStruct {
			int value;
		};

		MyStruct foo { .value = -1 };
		MyStruct bar { .value = -20 };
		gen::swap(foo, bar);

		REQUIRE(foo.value == -20);
		REQUIRE(bar.value == -1);
	}
}