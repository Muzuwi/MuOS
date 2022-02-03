#pragma once

#include <LibFormat/Formatter.hpp>
#include <LibFormat/Hex.hpp>

namespace Format {
	template<typename T>
	struct _IntegralFormatter {
	private:
		constexpr void format_dec(FormatContext& context, T const& value) {
			char buffer[32] {};
			size_t count = 0;

			T current = value;
			do {
				const auto rem = current % 10;
				const auto abs = rem < 0 ? -1 * rem : rem;
				current /= 10;
				buffer[count++] = '0' + abs;
			} while(current != 0);

			if(value < 0) {
				context.append_char('-');
			}
			for(unsigned i = 0; i < count; ++i) {
				context.append_char(buffer[count - 1 - i]);
			}
		}

		constexpr void format_hex(FormatContext& context, T const& value) {
			for(unsigned i = 0; i < sizeof(T); ++i) {
				const auto byte = (value >> (sizeof(T) - 1 - i) * 8) & 0xFF;
				const auto high = Format::nibble_to_hex_lowercase(byte >> 4u);
				const auto low = Format::nibble_to_hex_lowercase(byte & 0x0Fu);
				context.append_char(high);
				context.append_char(low);
			}
		}

	public:
		bool m_hex_form;

		constexpr void parse(ParserContext& context) {
			for(unsigned i = 0; i < context.count; ++i) {
				if(context.start[i] == 'x') {
					m_hex_form = true;
				}
			}
		}

		constexpr void format(FormatContext& context, T const& value) {
			if(m_hex_form) {
				format_hex(context, value);
			} else {
				format_dec(context, value);
			}
		}

	};

	template<>
	struct Formatter<signed char> {
		_IntegralFormatter<signed char> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, signed char const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<unsigned char> {
		_IntegralFormatter<unsigned char> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, unsigned char const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<short> {
		_IntegralFormatter<short> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, short const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<unsigned short> {
		_IntegralFormatter<unsigned short> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, unsigned short const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<int> {
		_IntegralFormatter<int> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, int const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<unsigned int> {
		_IntegralFormatter<unsigned int> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, unsigned int const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<long int> {
		_IntegralFormatter<long int> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, long int const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<long unsigned int> {
		_IntegralFormatter<long unsigned int> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, long unsigned int const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<long long int> {
		_IntegralFormatter<long long int> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, long long int const& value) {
			formatter.format(context, value);
		}
	};

	template<>
	struct Formatter<long long unsigned int> {
		_IntegralFormatter<long long unsigned int> formatter {};

		constexpr void parse(ParserContext& context) {
			formatter.parse(context);
		}

		constexpr void format(FormatContext& context, long long unsigned int const& value) {
			formatter.format(context, value);
		}
	};

}
