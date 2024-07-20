#include <catch2/catch.hpp>
#include <LibAllocator/BumpAllocator.hpp>

TEST_CASE("liballoc::BumpAlocator", "[liballoc]") {
	liballoc::Arena arena { (void*)0x80000000, 0x10000 };
	liballoc::BumpAllocator ba { arena };

	SECTION("simple allocations work") {
		auto* p = ba.allocate(0x1000);
		REQUIRE(p == (void*)0x80000000);

		auto* p2 = ba.allocate(0x4000);
		REQUIRE(p2 == (void*)0x80001000);
	}

	SECTION("allocation too big for arena") {
		auto* p = ba.allocate(0x10001);
		REQUIRE(p == nullptr);
	}

	SECTION("allocation equals exact arena size") {
		auto* p = ba.allocate(0x10000);
		REQUIRE(p == (void*)0x80000000);
	}
}
