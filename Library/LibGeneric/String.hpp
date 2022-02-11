#pragma once
#include <LibGeneric/TypeTraits/CharTraits.hpp>
#include <LibGeneric/Vector.hpp>

namespace gen {
	template<class CharType, template<typename> class Alloc = gen::Allocator>
	class BasicString : protected Vector<CharType, Alloc> {
		typedef char_traits<CharType> Traits;

		typedef typename Alloc<CharType>::template rebind<CharType>::other ObjAllocType;
		typename AllocatorTraits<ObjAllocType>::allocator_type _allocator;

		void _buffer_append(CharType const* cstr) {
			auto len = Traits::length(cstr);
			auto old_size = size();
			resize(old_size + len);
			Traits::copy(&at(old_size), cstr, len);
		}
		void _buffer_append(BasicString const& str) {
			auto len = str.size();
			auto old_size = size();
			resize(old_size + len);
			Traits::copy(&at(old_size), &str.at(0), len);
		}

		CharType* _bufptr() {
			return &Vector<CharType, Alloc>::at(0);
		}

		CharType const* _bufptr() const {
			return &Vector<CharType, Alloc>::at(0);
		}

		static size_t _find(CharType const* buf1, size_t n1, CharType const* buf2, size_t n2) {
			auto r = Traits::find(buf1, n1, buf2, n2);
			if(r == n1)
				return npos;
			else
				return r;
		}

	public:
		static constexpr size_t npos = -1;

		typedef CharType* iterator;
		typedef CharType const* const_iterator;

		BasicString() noexcept
		: Vector<CharType,Alloc>() {}

		BasicString(CharType const* cstr) noexcept
		: Vector<CharType,Alloc>() {
			_buffer_append(cstr);
		}

		BasicString(BasicString const& str) noexcept
		: Vector<CharType, Alloc>(str) {}

		~BasicString() {}

		/*
		 *  Getters
		 */
		CharType& at(size_t n) {
			return Vector<CharType, Alloc>::at(n);
		}
		CharType const& at(size_t n) const {
			return Vector<CharType, Alloc>::at(n);
		}

		CharType& operator[](size_t n) {
			return Vector<CharType, Alloc>::operator[](n);
		}
		CharType const& operator[](size_t n) const {
			return Vector<CharType, Alloc>::operator[](n);
		}

		CharType& front() {
			return Vector<CharType, Alloc>::at(0);
		}
		CharType const& front() const {
			return Vector<CharType, Alloc>::at(0);
		}
		CharType& back() {
			return Vector<CharType, Alloc>::at(size()-1);
		}
		CharType const& back() const {
			return Vector<CharType, Alloc>::at(size()-1);
		}

		bool empty() const {
			return Vector<CharType, Alloc>::empty();
		}

		size_t size() const {
			return Vector<CharType, Alloc>::size();
		}

		size_t capacity() const {
			return Vector<CharType, Alloc>::capacity();
		}

		/*
		 *  Iterators
		 */
		iterator begin() {
			return Vector<CharType, Alloc>::begin();
		}

		const_iterator begin() const {
			return Vector<CharType, Alloc>::begin();
		}

		iterator end() {
			return Vector<CharType, Alloc>::end();
		}

		const_iterator end() const {
			return Vector<CharType, Alloc>::end();
		}

		/*
		 *  Mutators
		 */
		void reserve(size_t n) {
			Vector<CharType, Alloc>::reserve(n);
		}

		void resize(size_t n, CharType const& v = CharType()) {
			Vector<CharType, Alloc>::resize(n, v);
		}

		void clear() {
			Vector<CharType, Alloc>::clear();
		}


		size_t find(BasicString const& pattern, size_t pos = 0) const {
			if(pos >= size())
				return npos;
			auto len = size() - pos;
			auto idx = _find(_bufptr()+pos, len, pattern._bufptr(), pattern.size());
			if (idx != npos)
				idx += pos;
			return idx;
		}

		size_t find(CharType const* buf, size_t pos, size_t count) const {
			if(pos >= size())
				return npos;
			auto len = size() - pos;
			auto idx = _find(_bufptr()+pos, len, buf, count);
			if (idx != npos)
				idx += pos;
			return idx;
		}

		BasicString& operator+=(BasicString const& str) {
			_buffer_append(str);
			return *this;
		}

		BasicString& operator+=(CharType const* cstr) {
			_buffer_append(cstr);
			return *this;
		}

		BasicString& operator+=(CharType ch) {
			Vector<CharType, Alloc>::push_back(ch);
			return *this;
		}

		bool operator==(BasicString const& str) const {
			if(size() != str.size())
				return false;
			return !Traits::compare(_bufptr(), str._bufptr(), size());
		}

		bool operator==(CharType const* cstr) const {
			if(size() != Traits::length(cstr))
				return false;
			return !Traits::compare(_bufptr(), cstr, size());
		}

		BasicString& operator=(BasicString const& other) {
			Vector<CharType, Alloc>::operator=(other);
			return *this;
		}

		//  FIXME: Completely wrong but for now it'll work
		char const* to_c_string() {
			if(empty()) {
				return "\0";
			}

			Vector<CharType, Alloc>::push_back('\0');
			return &at(0);
		}
	};

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	BasicString<CharType, Alloc> operator+(BasicString<CharType, Alloc> const& str, BasicString<CharType, Alloc> const& str2) {
		BasicString<CharType, Alloc> tmp {str};
		tmp += str2;
		return tmp;
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	BasicString<CharType, Alloc> operator+(BasicString<CharType, Alloc> const& str, CharType const* str2) {
		BasicString<CharType, Alloc> tmp {str};
		tmp += str2;
		return tmp;
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	class BasicStringView {
		typedef char_traits<CharType> Traits;
		typedef BasicString<CharType,Alloc> StrType;

		StrType const& m_str;
		size_t m_index;
		size_t m_len;
	public:
		typedef typename StrType::iterator iterator;
		typedef typename StrType::const_iterator const_iterator;

		BasicStringView(StrType const& str, size_t i, size_t len)
		: m_str(str), m_index(i), m_len(len) {}

		~BasicStringView() {}

		const_iterator begin() const {
			return &m_str.at(m_index);
		}

		const_iterator end() const {
			return &m_str.at(m_index) + m_len;
		}

		size_t size() const {
			return m_len;
		}

		bool empty() const {
			return m_len == 0;
		}

		CharType const& at(size_t n) const {
			return m_str.at(m_index+n);
		}

		CharType const& front() const {
			return m_str.operator[](m_index);
		}

		CharType const& back() const {
			return m_str.operator[](m_index+m_len-1);
		}

		int compare(BasicStringView const& view) const {
			if(view.size() < size())
				return -1;
			else if(view.size() > size())
				return 1;

			return Traits::compare(&m_str.at(m_index), &view.at(view.m_index), size());
		}

		int compare(StrType const& str) const {
			if(str.size() < size())
				return -1;
			else if(str.size() > size())
				return 1;

			return Traits::compare(&m_str.at(m_index), &str.at(0), size());
		}

		int compare(CharType const* cstr) const {
			auto len = Traits::length(cstr);
			if(len < size())
				return -1;
			else if(len > size())
				return 1;

			return Traits::compare(&m_str.at(m_index), cstr, size());
		}
	};

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	bool operator==(BasicStringView<CharType,Alloc> const& v1, BasicStringView<CharType,Alloc> const& v2) {
		return v1.compare(v2) == 0;
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	bool operator==(BasicStringView<CharType,Alloc> const& view, CharType const* cstr) {
		return view.compare(cstr) == 0;
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	bool operator==(BasicString<CharType,Alloc> const& str, BasicStringView<CharType,Alloc> const& view) {
		return view.compare(str) == 0;
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	bool operator==(BasicStringView<CharType,Alloc> const& view, BasicString<CharType,Alloc> const& str) {
		return view.compare(str) == 0;
	}

	typedef BasicString<char> String;
	typedef BasicStringView<char> StringView;
}


