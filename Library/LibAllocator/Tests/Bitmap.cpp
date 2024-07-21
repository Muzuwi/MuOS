#include <catch2/catch.hpp>
#include <cstring>
#include <LibAllocator/Bitmap.hpp>

TEST_CASE("liballoc::Bitmap", "[liballoc]") {
	uint8_t buffer[256] = {};

	SECTION("simple finding") {
		size_t idx = 0;
		REQUIRE(liballoc::bitmap_find_one(buffer, sizeof(buffer), idx));
		REQUIRE(idx == 0);
	}

	SECTION("finding when bitmap is full") {
		std::memset(buffer, 0xFF, sizeof(buffer));
		size_t idx = 0;
		REQUIRE(!liballoc::bitmap_find_one(buffer, sizeof(buffer), idx));
	}

	SECTION("finding single bit with last one clear") {
		std::memset(buffer, 0xFF, sizeof(buffer));
		buffer[255] = 0xFE;

		size_t idx = 0;
		REQUIRE(liballoc::bitmap_find_one(buffer, sizeof(buffer), idx));
		REQUIRE(idx == 2047);
	}

	SECTION("setting bits") {
		liballoc::bitmap_set(buffer, sizeof(buffer), 0, true);
		REQUIRE(buffer[0] == 0x80);

		liballoc::bitmap_set(buffer, sizeof(buffer), 0, false);
		REQUIRE(buffer[0] == 0x0);

		liballoc::bitmap_set(buffer, sizeof(buffer), 7, true);
		REQUIRE(buffer[0] == 0x1);

		liballoc::bitmap_set(buffer, sizeof(buffer), 7, false);
		REQUIRE(buffer[0] == 0x0);
	}
}