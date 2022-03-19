#include <LibGeneric/List.hpp>
#include <catch2/catch.hpp>

TEST_CASE("gen::List", "[structs]") {
	SECTION("default constructed list is empty") {
		gen::List<uint32_t> list {};

		REQUIRE(list.empty());
	}

	SECTION("pushing an element to the back") {
		gen::List<uint32_t> list {};
		list.push_back(111);

		REQUIRE(!list.empty());
		SECTION("peeking the back-element") {
			list.push_back(222);
			list.push_back(333);
			list.push_back(444);

			REQUIRE(list.back() == 444);
		}
	}

	SECTION("pushing an element to the front") {
		gen::List<uint32_t> list {};
		list.push_back(111);
		list.push_back(222);
		list.push_back(333);
		list.push_front(555);

		REQUIRE(!list.empty());
		SECTION("peeking the front element") {
			REQUIRE(list.front() == 555);
		}
	}

	SECTION("popping the front element") {
		gen::List<uint32_t> list {};
		list.push_back(111);
		list.push_back(222);
		list.push_back(333);
		list.pop_front();

		REQUIRE(!list.empty());
		REQUIRE(list.front() == 222);
	}

	SECTION("popping the back element") {
		gen::List<uint32_t> list {};
		list.push_back(444);
		list.push_back(555);
		list.push_back(666);
		list.pop_back();

		REQUIRE(!list.empty());
		REQUIRE(list.back() == 555);
	}

	SECTION("calculate size") {
		gen::List<uint32_t> list {};
		list.push_back(777);
		list.push_back(888);
		list.push_back(999);
		list.push_back(555);

		REQUIRE(list.size() == 4);
	}

	SECTION("clear") {
		gen::List<uint32_t> list {};
		list.push_back(777);
		list.push_back(888);
		list.push_back(999);
		list.push_back(555);
		list.clear();
		REQUIRE(list.empty());
	}

	SECTION("copy constructed list contains same values") {
		gen::List<uint32_t> list {};
		list.push_back(111);
		list.push_back(222);
		list.push_back(333);
		list.push_back(444);

		auto copy = gen::List<uint32_t> { list };
		REQUIRE(!copy.empty());
		REQUIRE(copy.size() == list.size());

		const uint32_t elements[] = { 111, 222, 333, 444 };
		unsigned which = 0;
		for(auto& element : copy) {
			REQUIRE(which < sizeof(elements));
			REQUIRE(element == elements[which]);
			which++;
		}
	}

	SECTION("iterators") {
		gen::List<uint32_t> list {};
		list.push_back(111);
		list.push_back(222);
		list.push_back(333);
		list.push_back(444);

		REQUIRE(list.begin() != list.end());
		SECTION("::begin()") {
			REQUIRE(*list.begin() == 111);
		}

		SECTION("advance the iterator (pre-increment)") {
			auto it = list.begin();
			++it;
			REQUIRE(it != list.end());
			REQUIRE(*it == 222);
		}

		SECTION("advance the iterator (post-increment)") {
			auto it = list.begin();
			it++;
			REQUIRE(it != list.end());
			REQUIRE(*it == 222);
		}

		SECTION("backwards iterator") {
			auto it = list.begin();
			it++;
			it++;
			REQUIRE(it != list.end());
			REQUIRE(*it == 333);
			it--;
			REQUIRE(it != list.end());
			REQUIRE(*it == 222);
		}

		SECTION("for-begin-end-loop") {
			const uint32_t elements[] = { 111, 222, 333, 444 };
			unsigned which = 0;
			for(auto it = list.begin(); it != list.end(); ++it) {
				REQUIRE(which < sizeof(elements));
				REQUIRE(*it == elements[which]);
				which++;
			}
		}
	}

	SECTION("insert") {
		gen::List<uint32_t> list {};
		list.push_back(111);
		list.push_back(222);
		list.push_back(333);
		list.push_back(444);

		auto it = list.begin();
		it++;
		it++;
		list.insert(it, 555);

		const auto size = list.size();
		const uint32_t elements[] = { 111, 222, 555, 333, 444 };
		unsigned which = 0;
		for(auto const& value : list) {
			REQUIRE(which < sizeof(elements));
			REQUIRE(value == elements[which]);
			which++;
		}
	}

	SECTION("erase") {
		gen::List<uint32_t> list {};
		list.push_back(111);
		list.push_back(222);
		list.push_back(333);
		list.push_back(444);

		auto it = list.begin();
		it++;
		it++;
		list.erase(it);

		const auto size = list.size();
		const uint32_t elements[] = { 111, 222, 444 };
		unsigned which = 0;
		for(auto const& value : list) {
			REQUIRE(which < size);
			REQUIRE(value == elements[which]);
			which++;
		}
	}
}