#include <catch2/catch.hpp>
#include <cstdint>
#include <LibGeneric/Optional.hpp>

TEST_CASE("gen::Optional", "[structs]") {
	SECTION("default constructed has no value") {
		gen::Optional<int> a {};
		REQUIRE(!a.has_value());
	}

	SECTION("nullopt direct initialized has no value") {
		gen::Optional<int> a { gen::nullopt };
		REQUIRE(!a.has_value());
	}

	SECTION("value-initialized has value") {
		gen::Optional<int> a { 5 };
		REQUIRE(a.has_value());
	}

	SECTION("direct initialized copy") {
		gen::Optional<int> a { 5 };
		gen::Optional<int> b { a };

		REQUIRE(a.has_value());
	}
}