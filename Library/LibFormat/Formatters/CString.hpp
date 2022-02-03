#pragma once

#include <LibFormat/Formatter.hpp>

namespace Format {
	template<>
	struct Formatter<char> {
		constexpr void parse(ParserContext&) {

		}

		constexpr void format(FormatContext& context, char const& value) {
			context.append_char(value);
		}
	};

	template<>
	struct Formatter<char const*> {
		constexpr void parse(ParserContext&) {

		}

		constexpr void format(FormatContext& context, char const*& value) {
			context.append_string(value);
		}
	};
}
