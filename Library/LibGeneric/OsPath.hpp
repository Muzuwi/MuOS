#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <LibGeneric/Basic_StringView.hpp>

#ifdef __is_kernel_build__
#include <Kernel/Debug/kassert.hpp>
#endif

namespace gen {
	class OsPathIterator {
		const char* m_ptr;

		static const char* _find_next_separator(const char* ptr) {
			if(*ptr == '/')
				ptr++;

			while(*ptr != '\0' && *ptr != '/')
				ptr++;

			return ptr;
		}

		static const char* _find_next_subpath(const char* ptr) {
			ptr = _find_next_separator(ptr);

			//  Skip until first non-separator byte in the path
			while(*ptr == '/')
				ptr++;

			return ptr;
		}
	public:
		OsPathIterator(const char* ptr)
		: m_ptr(ptr) {
			assert(ptr);
		}

		bool operator!=(const OsPathIterator& v) {
			return v.m_ptr != m_ptr;
		}

		OsPathIterator& operator++() {
			m_ptr = _find_next_subpath(m_ptr);
			return *this;
		}

		gen::ConstStringView operator*() const {
			return gen::ConstStringView {m_ptr, static_cast<size_t>(_find_next_separator(m_ptr) - m_ptr)};
		}
	};


	class OsPath {
		const char* m_path;
		OsPath* m_custody;
	public:
		OsPath(const char* path)
		: m_path(path), m_custody(nullptr) {}

		const char* path() const { return m_path; }
		OsPath* custody() const { return m_custody; }

		bool is_absolute() const {
			assert(m_path);
			return m_path[0] == '/';
		}

		OsPathIterator begin() const {
			return OsPathIterator{m_path};
		}

		OsPathIterator end() const {
			return OsPathIterator{m_path+strlen(m_path)};
		}
	};
}