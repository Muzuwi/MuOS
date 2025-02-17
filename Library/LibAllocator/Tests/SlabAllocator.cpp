#include <catch2/catch.hpp>
#include <cstdint>
#include <cstring>
#include <format>
#include <LibAllocator/SlabAllocator.hpp>

static constexpr const size_t ARENA_LEN = 32 * 1024 * 1024;
static std::vector<uint8_t> s_arena(ARENA_LEN);

TEST_CASE("liballoc::SlabAllocator", "[liballoc]") {
	SECTION("tests for different arena/object size combinations") {
		auto arena_size = GENERATE(as<size_t> {}, 0x8000, 0x10000, 0x20000, 0x40000, 0x80000);
		SECTION(std::format("arena size: {}", arena_size)) {
			auto object_size = GENERATE(as<size_t> {}, 8, 16, 32, 64, 128, 256, 1024, 2048, 4096);
			SECTION(std::format("object size: {}", object_size)) {
				std::memset(s_arena.data(), 0x0, arena_size);
				liballoc::Arena arena { s_arena.data(), arena_size };
				liballoc::SlabAllocator sa { arena, object_size };

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

				//  A more specialized test that performs allocations in a loop
				//  Each allocation must be within the bounds of the arena
				//	Eventually the resources must be exhausted. This test can of
				//  course fall into an infinite loop, in which case the underlying
				//  allocator is buggy.
				SECTION("allocation until resource exhaustion") {
					while(true) {
						auto* p = sa.allocate();
						if(!p) {
							SUCCEED("allocator eventually exhausts its resources");
							break;
						}

						SECTION("allocated pointers should be within pool area") {
							auto* pool_end = (uint8_t*)sa.pool_start() + sa.pool_capacity() * sa.object_size();
							if(!(p >= sa.pool_start() && p < pool_end)) {
								FAIL("the allocator has returned a pointer outside the pool area");
								break;
							}
						}
					}
				}
			}
		}
	}

	SECTION("allocation with excess bitmap elements") {
		static constexpr size_t arena_size = 4096;
		static constexpr size_t object_size = 2048;
		std::memset(s_arena.data(), 0x0, arena_size);
		liballoc::Arena arena { s_arena.data(), arena_size };
		liballoc::SlabAllocator sa { arena, object_size };

		//  First allocation will succeed
		auto* p1 = sa.allocate();
		REQUIRE(p1 != nullptr);
		REQUIRE(p1 >= arena.base);
		REQUIRE(p1 < arena.end());

		//  Second allocation MUST fail
		//  Due to the bitmap being part of the arena, the allocator
		//  cannot fit two objects of 2048-bytes. Only one can be
		//  allocated.
		auto* p2 = sa.allocate();
		REQUIRE(p2 == nullptr);
	}
}
