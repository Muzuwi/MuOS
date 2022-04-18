#pragma once
#include <stddef.h>
#include <stdint.h>

namespace _trait_space {
	template<class CharType>
	struct char_traits {
		constexpr static int compare(const CharType*, const CharType*, size_t n);
		constexpr static CharType* copy(CharType*, const CharType*, size_t, size_t);
		constexpr static size_t length(const CharType*);
		constexpr static size_t find(const CharType* str, size_t len, const CharType* pattern, size_t n);
	};

	template<class CharType>
	constexpr int char_traits<CharType>::compare(const CharType* a, const CharType* b, size_t n) {
		for(size_t i = 0; i < n; i++) {
			if(a[i] < b[i])
				return -1;
			else if(b[i] < a[i])
				return 1;
		}
		return 0;
	}

	template<class CharType>
	constexpr CharType* char_traits<CharType>::copy(CharType* dst, const CharType* src, size_t n, size_t dst_offset) {
		for(size_t i = 0; i < n; i++)
			dst[i + dst_offset] = src[i];
		return dst;
	}

	template<class CharType>
	constexpr size_t char_traits<CharType>::length(const CharType* a) {
		size_t i = 0;
		while(a && a[i] != CharType())
			i++;
		return i;
	}

	template<class CharType>
	constexpr size_t char_traits<CharType>::find(const CharType* str, size_t len, const CharType* pattern, size_t n) {
		size_t i = 0;
		while(i < len) {
			size_t j = 0;
			while(i + j < len && j < n && (str[i + j] == pattern[j]))
				j++;
			if(j == n)
				break;

			if(j > 0)
				i += j;
			else
				i++;
		}

		return i;
	}
}

/*
 *  Char specialization
 */
namespace gen {
	template<class CharType>
	struct char_traits : public _trait_space::char_traits<CharType> {};

	template<>
	struct char_traits<char> {
		constexpr static int compare(const char* a, const char* b, size_t n) {
			return _trait_space::char_traits<char>::compare(a, b, n);
		}

		constexpr static char* copy(char* a, const char* b, size_t n, size_t o = 0) {
			return _trait_space::char_traits<char>::copy(a, b, n, o);
		}

		constexpr static size_t length(const char* a) { return _trait_space::char_traits<char>::length(a); }

		constexpr static size_t find(const char* str, size_t len, const char* pattern, size_t n) {
			return _trait_space::char_traits<char>::find(str, len, pattern, n);
		}
	};
}
