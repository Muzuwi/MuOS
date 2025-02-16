#include <cstdint>
#include <LibGeneric/Utility.hpp>
#include <catch2/catch.hpp>

TEST_CASE("gen::Bitcast", "[bitcast]") {
	SECTION("trivial bitcast works") {
		struct Test {
			uint32_t value;
		} __attribute__((packed));

		const uint32_t value { 0xdeadbabe };
		const auto casted = gen::bitcast<Test>(value);

		REQUIRE(value == casted.value);
	}
}
