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

	SECTION("value getters") {
		gen::Optional<int> a { 5 };
		SECTION("unwrap") {
			REQUIRE(a.unwrap() == 5);
		}
		SECTION("unwrap_or_default") {
			REQUIRE(a.unwrap_or_default(5345) == 5);
			gen::Optional<int> b {};
			REQUIRE(b.unwrap_or_default(5345) == 5345);
		}
	}

	SECTION("direct initialized copy") {
		gen::Optional<int> a { 5 };
		gen::Optional<int> b { a };

		REQUIRE(a.has_value());
		REQUIRE(b.has_value());
		REQUIRE(a.unwrap() == 5);
		REQUIRE(b.unwrap() == 5);
	}

	SECTION("emplace") {
		gen::Optional<int> a {};
		REQUIRE(!a.has_value());
		a.emplace(5);
		REQUIRE(a.has_value());
		REQUIRE(a.unwrap() == 5);
	}

	SECTION("operator=") {
		gen::Optional<int> a { 1337 };
		REQUIRE(a.has_value());

		SECTION("with nullopt") {
			a = gen::nullopt;
			REQUIRE(!a.has_value());
		}

		SECTION("with value") {
			a = 5;
			REQUIRE(a.has_value());
			REQUIRE(a.unwrap() == 5);
		}

		SECTION("with non-null lvalue") {
			auto v = gen::Optional<int> { 46 };
			a = v;
			REQUIRE(a.has_value());
			REQUIRE(a.unwrap() == 46);
		}

		SECTION("with non-null rvalue") {
			a = gen::Optional<int> { 36 };
			REQUIRE(a.has_value());
			REQUIRE(a.unwrap() == 36);
		}
	}

	SECTION("reset") {
		gen::Optional<int> a { 1337 };
		REQUIRE(a.has_value());
		a.reset();
		REQUIRE(!a.has_value());
	}
}