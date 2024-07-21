#include <catch2/catch.hpp>
#include <cstdint>
#include <cstring>
#include <LibAllocator/SlabAllocator.hpp>

static constexpr const size_t ARENA_LEN = 0x10000;
static uint8_t s_arena[ARENA_LEN] = {};

TEST_CASE("liballoc::SlabAllocator", "[liballoc]") {
	auto arena_size = GENERATE(as<size_t> {}, 8, 16, 32, 64, 128, 256);
	liballoc::Arena arena { s_arena, ARENA_LEN };
	liballoc::SlabAllocator sa { arena, arena_size };

	SECTION("simple allocations should work") {
		auto* p = sa.allocate();
		REQUIRE(p != nullptr);
		std::memset(p, 0xDA, sa.object_size());

		auto* p2 = sa.allocate();
		REQUIRE(p2 != nullptr);
		REQUIRE(p != p2);
		std::memset(p2, 0xDA, sa.object_size());

		auto* p3 = sa.allocate();
		REQUIRE(p3 != nullptr);
		REQUIRE(p != p2);
		REQUIRE(p != p3);
		std::memset(p3, 0xDA, sa.object_size());

		SECTION("freeing allocated pointer works") {
			sa.free(p);

			auto* p2 = sa.allocate();
			REQUIRE(p2 != nullptr);
			REQUIRE(p == p2);
		}

		SECTION("allocated pointers should be within pool area") {
			auto* pool_end = (uint8_t*)sa.pool_start() + sa.pool_capacity() * sa.object_size();

			REQUIRE(p >= sa.pool_start());
			REQUIRE(p < pool_end);

			REQUIRE(p2 >= sa.pool_start());
			REQUIRE(p2 < pool_end);

			REQUIRE(p3 >= sa.pool_start());
			REQUIRE(p3 < pool_end);
		}
	}

	SECTION("alignment") {
		SECTION("pool starting address must be aligned to object size") {
			REQUIRE((reinterpret_cast<uintptr_t>(sa.pool_start()) & (8 - 1)) == 0);
		}

		SECTION("allocated pointers are aligned") {
			auto* p = sa.allocate();
			REQUIRE(p != nullptr);
			REQUIRE((reinterpret_cast<uintptr_t>(p) & (8 - 1)) == 0);

			auto* p2 = sa.allocate();
			REQUIRE(p2 != nullptr);
			REQUIRE((reinterpret_cast<uintptr_t>(p2) & (8 - 1)) == 0);

			auto* p3 = sa.allocate();
			REQUIRE(p3 != nullptr);
			REQUIRE((reinterpret_cast<uintptr_t>(p3) & (8 - 1)) == 0);

			auto* p4 = sa.allocate();
			REQUIRE(p4 != nullptr);
			REQUIRE((reinterpret_cast<uintptr_t>(p4) & (8 - 1)) == 0);
		}
	}

	SECTION("dividing the arena between bitmap and slab pool") {
		auto* bitmap = reinterpret_cast<uint8_t*>(sa.bitmap_start());
		size_t bitmap_size = sa.bitmap_size();
		auto* bitmap_end = bitmap + bitmap_size;

		auto* pool = reinterpret_cast<uint8_t*>(sa.pool_start());
		size_t pool_capacity = sa.pool_capacity();
		auto* pool_end = pool + pool_capacity * sa.object_size();

		SECTION("bitmap and pool do not overlap anywhere") {
			REQUIRE(bitmap_end <= pool);
		}

		SECTION("bitmap is entirely in the arena") {
			REQUIRE(bitmap >= arena.base);
			REQUIRE(bitmap_end <= arena.end());
		}

		SECTION("pool is entirely in the arena") {
			REQUIRE(pool >= arena.base);
			REQUIRE(pool_end <= arena.end());
		}
	}
}
