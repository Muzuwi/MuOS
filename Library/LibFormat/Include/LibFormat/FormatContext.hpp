#pragma once

#include <stddef.h>

namespace Format {
	struct FormatContext {
		enum class _FormatterState {
			RawOutput,
			LBraceEscape,
			RBraceEscape,
			CollectFormat
		};

		char const* format;
		char* buf;
		size_t buf_len;
		size_t format_pos;
		size_t buf_pos;
		size_t written;
		_FormatterState state;

		constexpr char current() const { return format[format_pos]; }

		constexpr char const* current_ptr() const { return &format[format_pos]; }

		constexpr void next() { format_pos += 1; }

		constexpr bool is_null() const { return format[format_pos] == '\0'; }

		constexpr bool append_char(const char value) {
			written++;
			if(buf_pos >= buf_len) {
				return false;
			}
			if(buf_pos == buf_len - 1) {
				//  Null terminate properly and do not append more characters
				buf[buf_pos++] = '\0';
				return false;
			}

			buf[buf_pos++] = value;
			return true;
		}

		constexpr bool append_string(char const* cstr) {
			bool failed = false;
			while(*cstr != '\0') {
				const bool v = append_char(*cstr);
				failed |= !v;
				cstr++;
			}
			return failed;
		}
	};
}
