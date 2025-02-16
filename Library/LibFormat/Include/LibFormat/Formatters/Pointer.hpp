#pragma once

#include <LibFormat/Formatter.hpp>
#include <LibFormat/Hex.hpp>

namespace Format {
	//  Based on libfmt
	template<typename Target, typename Source>
	constexpr auto _format_bitcast(Source const& v) -> Target {
		static_assert(sizeof(Target) == sizeof(Source), "Type arguments for bitcast must be the same size.");
		auto target = Target();
		__builtin_memcpy(&target, &v, sizeof(Target));
		return target;
	}

	template<>
	struct Formatter<void*> {
		constexpr void parse(ParserContext&) {}

		constexpr void format(FormatContext& context, void*& value) {
			auto ptr_value = _format_bitcast<uintptr_t>(value);
			context.append_string("0x");
			for(unsigned i = 0; i < sizeof(uintptr_t); ++i) {
				const auto byte = (ptr_value >> (sizeof(uintptr_t) - 1 - i) * 8u) & 0xFFu;
				const auto high = Format::nibble_to_hex_lowercase(byte >> 4u);
				const auto low = Format::nibble_to_hex_lowercase(byte & 0x0Fu);
				context.append_char(high);
				context.append_char(low);
			}
		}
	};

	template<>
	struct Formatter<void const*> {
		constexpr void parse(ParserContext&) {}

		constexpr void format(FormatContext& context, void const*& value) {
			auto ptr_value = _format_bitcast<uintptr_t>(value);
			for(unsigned i = 0; i < sizeof(uintptr_t); ++i) {
				const auto byte = (ptr_value >> (sizeof(uintptr_t) - 1 - i) * 8u) & 0xFFu;
				const auto high = Format::nibble_to_hex_lowercase(byte >> 4u);
				const auto low = Format::nibble_to_hex_lowercase(byte & 0x0Fu);
				context.append_char(high);
				context.append_char(low);
			}
		}
	};

	/*
	 *  Helpers for formatting pointers
	 */
	template<typename T>
	constexpr void const* ptr(T const* v) {
		return _format_bitcast<void const*>(v);
	}

	template<typename T>
	constexpr void* ptr(T* v) {
		return _format_bitcast<void*>(v);
	}

}