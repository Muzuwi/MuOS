#include <catch2/catch.hpp>
#include <LibGeneric/StaticVector.hpp>

TEST_CASE("gen::StaticVector", "[structs]") {
	gen::StaticVector<unsigned, 32> v {};

	SECTION("empty on default construction") { REQUIRE(v.empty()); }
	SECTION("size on default construction") { REQUIRE(v.size() == 0); }
	SECTION("capacity") { REQUIRE(v.capacity() == 32); }

	SECTION("pushing objects") {
		v.push_back(5);
		v.push_back(10);
		v.push_back(15);
		v.push_back(20);

		SECTION("size after pushing objects") { REQUIRE(v.size() == 4); }
		SECTION("pushed values are correct") {
			REQUIRE(v[0] == 5);
			REQUIRE(v[1] == 10);
			REQUIRE(v[2] == 15);
			REQUIRE(v[3] == 20);
		}
	}

	SECTION("popping objects") {
		v.push_back(5);
		v.push_back(10);
		v.push_back(15);
		v.push_back(20);
		v.pop_back();
		v.pop_back();
		v.push_back(25);

		SECTION("size after pop") { REQUIRE(v.size() == 3); }
		SECTION("values are correct") {
			REQUIRE(v[0] == 5);
			REQUIRE(v[1] == 10);
			REQUIRE(v[2] == 25);
		}
	}

	SECTION("clearing vector") {
		v.push_back(5);
		v.push_back(10);
		v.push_back(15);
		v.push_back(20);
		v.clear();

		SECTION("size after clear") { REQUIRE(v.size() == 0); }
	}

	SECTION("front/back") {
		v.push_back(111);
		v.push_back(222);
		v.push_back(333);
		v.push_back(444);
		SECTION("front works") { REQUIRE(v.front() == 111); }
		SECTION("back works") { REQUIRE(v.back() == 444); }
	}

	SECTION("object construction") {
		struct Tester {
			uint64_t* cptr { nullptr };
			uint64_t* dptr { nullptr };
			uint64_t* mptr { nullptr };

			constexpr Tester() = default;

			constexpr explicit Tester(uint64_t* constr, uint64_t* destr, uint64_t* moves)
			    : cptr(constr)
			    , dptr(destr)
			    , mptr(moves) {
				if(cptr) {
					(*cptr)++;
				}
			}

			constexpr explicit Tester(Tester&& v)
			    : cptr(v.cptr)
			    , dptr(v.dptr)
			    , mptr(v.mptr) {
				if(mptr) {
					(*mptr)++;
				}
			}

			~Tester() {
				if(dptr) {
					(*dptr)++;
				}
			}
		};

		uint64_t constructions = 0, destructions = 0, moves = 0;
		gen::StaticVector<Tester, 16> constr {};

		for(unsigned i = 0; i < 16; ++i) {
			auto t = Tester { &constructions, &destructions, &moves };
			constr.push_back(gen::move(t));
		}
		SECTION("all values were moved") { REQUIRE(moves == 16); }
		constructions = 0;
		destructions = 0;
		for(unsigned i = 0; i < 4; ++i) {
			constr.pop_back();
		}
		SECTION("destructors were called for popped objects") { REQUIRE(destructions == 4); }

		constructions = 0;
		destructions = 0;
		constr.clear();
		SECTION("remaining object destructors were called for clear") { REQUIRE(destructions == 12); }
	}

	SECTION("iterators") {
		v.push_back(1);
		v.push_back(2);
		v.push_back(3);
		v.push_back(4);

		unsigned n = 0;
		for(auto const& value : v) {
			REQUIRE(value == (n + 1));
			n++;
		}
	}
}
