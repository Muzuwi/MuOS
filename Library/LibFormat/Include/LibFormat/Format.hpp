#pragma once

#include <LibFormat/FormatContext.hpp>
#include <LibFormat/Formatter.hpp>
#include <LibFormat/Formatters/Default.hpp>
#include <LibFormat/ParserContext.hpp>
#include <stddef.h>
#include <stdint.h>

namespace Format {
	template<typename T>
	concept Formattable = requires(Formatter<T> formatter, T value, ParserContext& parser_ctx,
	                               FormatContext& format_ctx) {
		Formatter<T> {};
		formatter.parse(parser_ctx);
		formatter.format(format_ctx, value);
	};

	constexpr void _format_impl(FormatContext& context) {
		context.state = FormatContext::_FormatterState::RawOutput;

		while(!context.is_null()) {
			const auto ch = context.current();
			context.next();

			switch(context.state) {
				case FormatContext::_FormatterState::RawOutput: {
					if(ch == '{') {
						context.state = FormatContext::_FormatterState::LBraceEscape;
					} else if(ch == '}') {
						context.state = FormatContext::_FormatterState::RBraceEscape;
					} else {
						context.append_char(ch);
					}
					break;
				}
				case FormatContext::_FormatterState::LBraceEscape: {
					if(ch == '{') {
						context.state = FormatContext::_FormatterState::RawOutput;
						context.append_char('{');
					} else if(ch == '}') {
						//  Format error: trailing format specifier
						context.state = FormatContext::_FormatterState::RawOutput;
					} else {
						//  This is a format error, but let the state machine progress
						//  as usual to handle trailing formats and skip over them as if nothing happened
						//  Format error: trailing format specifier
						context.state = FormatContext::_FormatterState::CollectFormat;
					}
					break;
				}
				case FormatContext::_FormatterState::RBraceEscape: {
					if(ch == '}') {
						context.state = FormatContext::_FormatterState::RawOutput;
						context.append_char('}');
					} else {
						//  Format error: found '}' without proceeding '{'
						context.state = FormatContext::_FormatterState::RawOutput;
					}
					break;
				}
				case FormatContext::_FormatterState::CollectFormat: {
					if(ch == '{') {
						//  Format error: nested '{' in format specifier
						context.state = FormatContext::_FormatterState::RawOutput;
					} else if(ch == '}') {
						//  Format error: trailing format specifier
						context.state = FormatContext::_FormatterState::RawOutput;
					} else {
						//  Remain in the current state until a '}' is encountered
						context.state = FormatContext::_FormatterState::CollectFormat;
					}
					break;
				}
				default: break;
			}
		}
		context.append_char('\0');
	}

	template<Formattable FmtArg, typename... Args>
	constexpr void _format_impl(FormatContext& context, FmtArg top, Args... others) {
		auto parser_context = ParserContext {};
		auto formatter = Formatter<FmtArg> {};

		context.state = FormatContext::_FormatterState::RawOutput;
		//  TODO: Document the formatter state machine
		while(!context.is_null()) {
			const auto ch = context.current();
			context.next();

			switch(context.state) {
				case FormatContext::_FormatterState::RawOutput: {
					if(ch == '{') {
						context.state = FormatContext::_FormatterState::LBraceEscape;
						parser_context.start = context.current_ptr();
					} else if(ch == '}') {
						context.state = FormatContext::_FormatterState::RBraceEscape;
					} else {
						context.append_char(ch);
					}
					break;
				}
				case FormatContext::_FormatterState::LBraceEscape: {
					if(ch == '{') {
						context.state = FormatContext::_FormatterState::RawOutput;
						context.append_char('{');
					} else if(ch == '}') {
						//  Empty format specifier
						formatter.parse(parser_context);
						formatter.format(context, top);
						_format_impl(context, others...);
						return;
					} else {
						parser_context.count++;
						context.state = FormatContext::_FormatterState::CollectFormat;
					}
					break;
				}
				case FormatContext::_FormatterState::RBraceEscape: {
					if(ch == '}') {
						context.state = FormatContext::_FormatterState::RawOutput;
						context.append_char('}');
					} else {
						//  Format error: found '}' without proceeding '{'
						context.state = FormatContext::_FormatterState::RawOutput;
					}
					break;
				}
				case FormatContext::_FormatterState::CollectFormat: {
					if(ch == '{') {
						//  Format error: nested '{' in format specifier
						context.state = FormatContext::_FormatterState::RawOutput;
					} else if(ch == '}') {
						//  Format specifier ending
						formatter.parse(parser_context);
						formatter.format(context, top);
						_format_impl(context, others...);
						return;
					} else {
						//  Remain in the current state until a '}' is encountered
						parser_context.count++;
						context.state = FormatContext::_FormatterState::CollectFormat;
					}
					break;
				}
				default: break;
			}
		}
	}

	template<typename... Args>
	constexpr size_t format(char const* format, char* output, size_t output_len, Args... args) {
		auto context = FormatContext { .format = format,
			                           .buf = output,
			                           .buf_len = output_len,
			                           .format_pos = 0,
			                           .buf_pos = 0,
			                           .written = 0,
			                           .state = FormatContext::_FormatterState::RawOutput };
		Format::_format_impl(context, args...);
		return context.written;
	}

}
