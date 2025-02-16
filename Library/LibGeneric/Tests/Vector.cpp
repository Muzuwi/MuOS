#include <cassert>
#include <initializer_list>
#include <LibGeneric/Vector.hpp>
#include <catch2/catch.hpp>
#include "DefConstructProbe.hpp"

struct VectorDestructorTest {
	static int object_count;
	unsigned dummy;

	VectorDestructorTest() {
		object_count++;
	}

	VectorDestructorTest(const VectorDestructorTest&) {
		object_count++;
	}

	VectorDestructorTest(VectorDestructorTest&&) {
		object_count++;
	}

	~VectorDestructorTest() {
		object_count--;
	}
};

int VectorDestructorTest::object_count { 0 };

TEST_CASE("gen::Vector", "[structs]") {
	SECTION("default constructed vector is empty") {
		gen::Vector<uint32_t> vec {};
		REQUIRE(vec.empty());
	}

	SECTION("pushing an element to the back") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);

		REQUIRE(!vec.empty());
		REQUIRE(vec[0] == 1111);
	}

	SECTION("popping an element from the back") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);
		vec.push_back(2222);
		vec.pop_back();

		REQUIRE(!vec.empty());
		REQUIRE(vec[0] == 1111);
	}

	SECTION("clearing") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);
		vec.clear();

		REQUIRE(vec.empty());
	}

	SECTION("popping an element from the back to empty") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);
		vec.pop_back();

		REQUIRE(vec.empty());
	}

	SECTION("size is correct") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);
		vec.push_back(2222);
		vec.push_back(3333);
		vec.push_back(4444);

		REQUIRE(vec.size() == 4);
	}

	SECTION("iterators (range-based for loop)") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);
		vec.push_back(2222);
		vec.push_back(3333);
		vec.push_back(4444);

		const uint32_t elements[] = { 1111, 2222, 3333, 4444 };
		unsigned which = 0;
		for(auto& value : vec) {
			REQUIRE(which < sizeof(elements));
			REQUIRE(vec[which] == value);
			which++;
		}
	}

	SECTION("copy constructor works") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);
		vec.push_back(2222);
		vec.push_back(3333);

		auto copy = gen::Vector { vec };
		REQUIRE(vec.size() == copy.size());
		for(unsigned i = 0; i < copy.size(); ++i) {
			REQUIRE(vec[i] == copy[i]);
		}
	}

	SECTION("access by ::at ") {
		gen::Vector<uint32_t> vec {};
		vec.push_back(1111);
		vec.push_back(2222);
		vec.push_back(3333);

		REQUIRE(vec.at(2) == 3333);
	}

	//  Taken verbatim from Catch2 documentation
	gen::Vector<int> v {};
	v.resize(5);

	REQUIRE(v.size() == 5);
	REQUIRE(v.capacity() >= 5);

	SECTION("resizing bigger changes size and capacity") {
		v.resize(10);

		REQUIRE(v.size() == 10);
		REQUIRE(v.capacity() >= 10);
	}

	SECTION("resizing smaller changes size but not capacity") {
		v.resize(0);

		REQUIRE(v.size() == 0);
		REQUIRE(v.capacity() >= 5);
	}

	SECTION("reserving bigger changes capacity but not size") {
		v.reserve(10);

		REQUIRE(v.size() == 5);
		REQUIRE(v.capacity() >= 10);
	}

	SECTION("reserving smaller does not change size or capacity") {
		v.reserve(0);

		REQUIRE(v.size() == 5);
		REQUIRE(v.capacity() >= 5);
	}

	SECTION("default construction on resize") {
		//  When resizing to a larger size, new elements are default constructed
		//  This checks that behaviour by using a helper class that sets a flag when
		//  it is constructed via its' default constructor.

		gen::Vector<DefConstructProbe> vec {};
		for(unsigned i = 0; i < 5; ++i) {
			vec.push_back(DefConstructProbe { 5 });
		}
		vec.resize(20);

		for(unsigned i = 0; i < vec.size(); ++i) {
			if(i < 5) {
				REQUIRE(!vec[i].defaulted);
			} else {
				REQUIRE(vec[i].defaulted);
			}
		}
	}

	SECTION("object construction/destruction") {
		VectorDestructorTest::object_count = 0;

		gen::Vector<VectorDestructorTest> vec {};
		vec.resize(20);

		SECTION("constructors called for all elements") {
			REQUIRE(VectorDestructorTest::object_count == 20);
		}

		SECTION("destructors called for all elements") {
			vec.clear();
			REQUIRE(VectorDestructorTest::object_count == 0);
		}
	}
}