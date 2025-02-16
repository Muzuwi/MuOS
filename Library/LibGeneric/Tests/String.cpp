#include <cassert>
#include <catch2/catch.hpp>
#include <cstring>
#include <LibGeneric/String.hpp>

constexpr gen::String test_const_eval() {
	gen::String str { "hello" };
	gen::String str2 { "world" };
	str.append(str2);
	return str;
}

static_assert(test_const_eval().size() == 10, "Hello");
static_assert(test_const_eval() == "helloworld", "Hello");

TEST_CASE("gen::String") {
	SECTION("default constructed") {
		gen::String str {};
		SECTION("size()") {
			REQUIRE(str.size() == 0);
		}
	}
	SECTION("c-string literal constructed \"hello\"") {
		gen::String str { "hello" };
		SECTION("size() == 5") {
			REQUIRE(str.size() == 5);
		}
	}
	SECTION("operator==") {
		gen::String str { "hello" };
		REQUIRE(str.size() == 5);
		REQUIRE(str == "hello");
	}
	SECTION("append") {
		gen::String str { "hello" };
		REQUIRE(str.size() == 5);

		SECTION("append BasicString") {
			gen::String str2 { "world" };
			str.append(str2);
			REQUIRE(str.size() == 10);
			REQUIRE(str == "helloworld");
		}
		SECTION("append c-string literal") {
			str.append("world");
			REQUIRE(str.size() == 10);
			REQUIRE(str == "helloworld");
		}
		SECTION("append character literal") {
			str.append('5');
			REQUIRE(str.size() == 6);
			REQUIRE(str == "hello5");
		}
		SECTION("operator+=") {
			SECTION("with BasicString") {
				str += gen::String { "world" };
				REQUIRE(str.size() == 10);
				REQUIRE(str == "helloworld");
			}
			SECTION("with c-string literal") {
				str += "world";
				REQUIRE(str.size() == 10);
				REQUIRE(str == "helloworld");
			}
			SECTION("with character literal") {
				str += 'c';
				REQUIRE(str.size() == 6);
				REQUIRE(str == "helloc");
			}
		}
		SECTION("self-append") {
			str.append(str);
			REQUIRE(str.size() == 10);
			REQUIRE(str == "hellohello");
		}
	}
	SECTION("operator=") {
		gen::String str { "hello" };
		REQUIRE(str.size() == 5);

		SECTION("with c-string literal") {
			str = "abc";
			REQUIRE(str.size() == 3);
			REQUIRE(str == "abc");
		}

		SECTION("with BasicStr") {
			str = gen::String { "zxcn" };
			REQUIRE(str.size() == 4);
			REQUIRE(str == "zxcn");
		}
	}
	SECTION("operator+") {
		gen::String str { "abc" };
		SECTION("BasicString + BasicString") {
			gen::String str2 { "def" };
			auto res = str + str2;
			REQUIRE(res.size() == 6);
			REQUIRE(res == "abcdef");
		}
		SECTION("BasicString + c-string") {
			auto res = str + "def";
			REQUIRE(res.size() == 6);
			REQUIRE(res == "abcdef");
		}
	}
	SECTION("clear") {
		gen::String str { "abc" };
		str.clear();
		REQUIRE(str.empty());
	}
	SECTION("resize") {
		gen::String str { "abc" };
		str.resize(10, '?');
		REQUIRE(str.size() == 10);
		REQUIRE(str == "abc???????");
	}
	SECTION("detail: small-string -> heap migration") {
		gen::String str { "abc" };
		REQUIRE(str.capacity() == 23);

		str.resize(32, '?');
		REQUIRE(str.size() == 32);
		REQUIRE(str == "abc?????????????????????????????");
		REQUIRE(str.capacity() == 32);
		str.reserve(64);
		REQUIRE(str.capacity() >= 64);
	}
	SECTION("data (c-string)") {
		gen::String str {};
		REQUIRE(std::strlen(str.data()) == 0);
		REQUIRE(std::strcmp(str.data(), "") == 0);
		str = "Hello!";
		REQUIRE(std::strlen(str.data()) == 6);
		REQUIRE(std::strcmp(str.data(), "Hello!") == 0);

		SECTION("detail: relocation on data() for max small strings") {
			str = "AAAAAAAAAAAAAAAAAAAAAAA";//  23 characters - max small string capacity
			REQUIRE(str.capacity() == 23);
			REQUIRE(std::strlen(str.data()) == 23);
			REQUIRE(str.capacity() > 23);
		}
	}
}