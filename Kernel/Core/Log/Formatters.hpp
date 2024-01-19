#pragma once
#include "Core/Error/Error.hpp"
#include "LibFormat/Format.hpp"
#include "LibFormat/FormatContext.hpp"
#include "LibFormat/ParserContext.hpp"
#include "LibGeneric/String.hpp"

namespace Format {
	//  FIXME: This specific formatter would better belong in LibFormatter instead
	template<>
	struct Formatter<gen::String> {
		constexpr void parse(ParserContext&) {}

		constexpr void format(FormatContext& context, gen::String const& value) {
			for(auto c : value) {
				context.append_char(c);
			}
		}
	};

	template<>
	struct Formatter<core::Error> {
		constexpr void parse(ParserContext&) {}

		constexpr void format(FormatContext& context, core::Error const& value) {
			char buf[8];
			auto n = Format::format("{}", buf, sizeof(buf), static_cast<size_t>(value));
			for(size_t i = 0; i < n; ++i) {
				context.append_char(buf[i]);
			}
		}
	};
}
