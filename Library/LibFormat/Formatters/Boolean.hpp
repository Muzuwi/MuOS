#pragma once

#include <LibFormat/Formatter.hpp>

namespace Format {
	template<>
	struct Formatter<bool> {
		constexpr void parse(ParserContext&) {

		}

		constexpr void format(FormatContext& context, bool const& value) {
			context.append_string(value ? "true" : "false");
		}
	};

}
