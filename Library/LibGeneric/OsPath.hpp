#pragma once
#include <LibGeneric/String.hpp>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace gen {
	class OsPathIterator {
		char const* m_ptr;

		static inline constexpr char const* find_next_separator(char const* ptr) {
			if(*ptr == '/')
				ptr++;

			while(*ptr != '\0' && *ptr != '/')
				ptr++;

			return ptr;
		}

		static inline constexpr char const* find_next_subpath(char const* ptr) {
			ptr = find_next_separator(ptr);

			//  Skip until first non-separator byte in the path
			while(*ptr == '/')
				ptr++;

			return ptr;
		}
	public:
		OsPathIterator(char const* path)
		    : m_ptr(path) {}

		bool operator!=(OsPathIterator const& v) { return v.m_ptr != m_ptr; }

		OsPathIterator& operator++() {
			m_ptr = find_next_subpath(m_ptr);
			return *this;
		}

		gen::StringView operator*() const {
			return gen::StringView { m_ptr, 0, static_cast<size_t>(find_next_separator(m_ptr) - m_ptr) };
		}
	};

	class OsPath {
		gen::String m_path;
	public:
		explicit OsPath(gen::String path_string) noexcept
		    : m_path(gen::move(path_string)) {}

		bool absolute() const {
			assert(!m_path.empty());
			return m_path[0] == '/';
		}

		OsPathIterator begin() const { return OsPathIterator { &m_path[0] }; }

		OsPathIterator end() const { return OsPathIterator { &m_path[0] + m_path.size() }; };
	};
}