#include <LibGeneric/SharedPtr.hpp>
#include <catch2/catch.hpp>

TEST_CASE("gen::SharedPtr") {
	SECTION("default constructed evaluates to false") {
		gen::SharedPtr<unsigned> t {};
		REQUIRE(!t);
	}

	SECTION("make_shared") {
		auto ptr = gen::make_shared<unsigned>(5);
		REQUIRE(ptr);
		REQUIRE(*ptr == 5);
	}

	SECTION("construct from raw pointer") {
		auto ptr = gen::SharedPtr<unsigned> { new(gen::Allocator<unsigned int>::allocate(1)) unsigned(5) };
		REQUIRE(ptr);
		REQUIRE(*ptr == 5);
	}

	SECTION("construct with operator=") {
		auto ptr = gen::make_shared<unsigned>(5);
		auto other = gen::SharedPtr<unsigned> {};
		other = ptr;
		REQUIRE(ptr.get() == other.get());
		REQUIRE(ptr.use_count() == 2);
	}

	SECTION("copy constructor") {
		auto ptr = gen::make_shared<unsigned>(5);
		SECTION("default refcount is 1") {
			REQUIRE(ptr.use_count() == 1);
		}

		{
			auto copy = gen::SharedPtr { ptr };

			SECTION("pointers point to the same object") {
				REQUIRE(&*ptr == &*copy);
			}

			SECTION("refcount is increased after copy") {
				REQUIRE(ptr.use_count() == 2);
			}
		}

		SECTION("refcount is decreased after scope drops") {
			REQUIRE(ptr.use_count() == 1);
		}

		SECTION("refcount and pointer is 0 after reset") {
			ptr.reset();
			REQUIRE(!ptr);
			REQUIRE(ptr.use_count() == 0);
		}
	}

}