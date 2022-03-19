#include <cassert>
#include <LibGeneric/BitMap.hpp>
#include <catch2/catch.hpp>

TEST_CASE("gen::Bitmap", "[structs]") {
	gen::BitMap bitmap { 1024 };

	SECTION("size is correct") {
		REQUIRE(bitmap.count() == 1024);
	}

	SECTION("set works") {
		bitmap.set(0, true);
		bitmap.set(1023, true);
		REQUIRE(bitmap.at(0) == true);
		REQUIRE(bitmap.at(1023) == true);
	}

	SECTION("clear works") {
		bitmap.clear();
		REQUIRE(bitmap.count() == 1024);
		REQUIRE(bitmap.at(0) == false);
		REQUIRE(bitmap.at(1023) == false);
	}

	bitmap.clear();

	//  ffffffff00000000.... pattern
	for(unsigned i = 0; i < 1024; ++i) {
		bitmap.set(i, (i / 32) % 2 == 0);
	}

	//  FIXME: Does this tie into the implementation a little bit too much?

	SECTION("finding one-bit sequences") {
		REQUIRE(bitmap.find_seq_clear(1) == 32);
	}

	SECTION("finding many-bit sequences") {
		REQUIRE(bitmap.find_seq_clear(1) == 32);
	}

}

