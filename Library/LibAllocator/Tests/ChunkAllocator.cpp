#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <LibAllocator/ChunkAllocator.hpp>

static constexpr const size_t ARENA_LEN = 0x10000;
static uint8_t s_arena[ARENA_LEN] = {};

TEST_CASE("liballoc::ChunkAllocator", "[liballoc]") {
	liballoc::Arena arena { s_arena, ARENA_LEN };
	liballoc::ChunkAllocator ca { arena };

	SECTION("simple allocations work") {
		auto* p = ca.allocate(0x1000);
		REQUIRE(p != nullptr);

		auto* p2 = ca.allocate(0x4000);
		REQUIRE(p2 != nullptr);
		REQUIRE(p2 != p);
		REQUIRE(p2 > p);

		SECTION("freeing works") {
			ca.free(p);
			ca.free(p2);

			auto* p3 = ca.allocate(0x1000);
			REQUIRE(p3 < p2);
		}
	}

	SECTION("allocation too big for arena") {
		auto* p = ca.allocate(0x10001);
		REQUIRE(p == nullptr);
	}

	SECTION("allocation equals exact arena size") {
		auto* p = ca.allocate(0x10000);
		REQUIRE(p == nullptr);
	}
}