#pragma once
#include <LibGeneric/TypeTraits/CharTraits.hpp>
#include <LibGeneric/Vector.hpp>

namespace gen {
	template<class CharType, template<typename> class Alloc = gen::Allocator>
	class __BasicStrStorage {
		typedef char_traits<CharType> Traits;
		typedef typename Alloc<CharType>::template rebind<CharType>::other ObjAllocType;
		[[no_unique_address]] typename AllocatorTraits<ObjAllocType>::allocator_type _allocator;

		struct __LongStringData {
			CharType* buffer;
			size_t count;
			size_t buffer_size;

			constexpr __LongStringData() = default;
		} __attribute__((packed));

		struct __SmallStringData {
			CharType string[sizeof(__LongStringData) - 1];
			struct {
				uint8_t size : 7;
				bool flag    : 1;
			} __attribute__((packed)) control;
			constexpr __SmallStringData() = default;
		} __attribute__((packed));

		static_assert(sizeof(__LongStringData) == sizeof(__SmallStringData));

		union {
			__LongStringData regular_string;
			__SmallStringData small_string;
		} m_storage;

		//  Theoretically this is sometimes undefined behaviour, as reading from an inactive union member is undefined.
		//  Hopefully you use a compiler that implements this "correctly".
		constexpr bool is_small_string() const { return m_storage.small_string.control.flag; }
		constexpr void set_small_string(bool is_small_string) { m_storage.small_string.control.flag = is_small_string; }

		constexpr CharType* buffer_allocate(size_t n) { return AllocatorTraits<ObjAllocType>::allocate(n); }
		constexpr void buffer_free(CharType* ptr, size_t n) {
			return AllocatorTraits<ObjAllocType>::deallocate(ptr, n);
		}

		constexpr void buffer_copy_from(CharType const* source, size_t count) {
			if(source == nullptr) {
				return;
			}

			auto* c = begin();
			for(auto* p = source; p < source + count; ++p) {
				*c = *p;
				++c;
			}
		}

		constexpr void buffer_reallocate(size_t new_size) {
			auto* old_buffer = m_storage.regular_string.buffer;
			auto old_size = m_storage.regular_string.buffer_size;

			m_storage.regular_string.buffer = buffer_allocate(new_size);
			m_storage.regular_string.buffer_size = new_size;

			buffer_copy_from(old_buffer, m_storage.regular_string.count);
			buffer_free(old_buffer, old_size);
		}

		constexpr void buffer_resize_if_needed(size_t requested_count) {
			if(m_storage.regular_string.count + requested_count < m_storage.regular_string.buffer_size) {
				return;
			}

			const auto required = m_storage.regular_string.count + requested_count;
			auto new_size = m_storage.regular_string.buffer_size;
			if(new_size == 0) {
				new_size = 1;
			}
			while(required >= new_size) {
				new_size *= 2;
			}
			buffer_reallocate(new_size);
		}

		constexpr void allocate_storage(size_t increment) {
			if(is_small_string()) {
				const auto old_size = size();
				const auto required = size() + increment;

				if(required <= sizeof(__SmallStringData::string)) {
					//  Nothing to do - small string has enough capacity for this allocation
					return;
				}

				//  A migration to a heap-allocated buffer is required
				auto* buffer = buffer_allocate(required);

				//  Copy over current contents
				auto* c = begin();
				for(auto* p = buffer; p < buffer + old_size; ++p) {
					*p = *c;
					++c;
				}

				set_small_string(false);
				m_storage.regular_string.buffer = buffer;
				m_storage.regular_string.buffer_size = required;
				m_storage.regular_string.count = old_size;
			} else {
				//  FIXME: Handle the incredibly unrealistic scenario of the capacity exceeding 0x7fffffffffffffff
				//         which would clobber the small string flag
				buffer_resize_if_needed(increment);
			}
		}

		constexpr void append_impl(CharType ch) {
			const auto count = size();
			allocate_storage(1);
			size_set(size() + 1);
			*(begin() + count) = ch;
		}

		constexpr void append_impl(CharType ch, size_t length) {
			const auto count = size();
			allocate_storage(length);
			size_set(size() + length);

			auto* c = begin() + count;
			for(auto i = 0u; i < length; ++i) {
				*c = ch;
				++c;
			}
		}

		constexpr void append_impl(CharType const* source, size_t length) {
			const auto count = size();
			allocate_storage(length);
			size_set(size() + length);

			auto* c = begin() + count;
			for(auto* p = source; p < source + length; ++p) {
				*c = *p;
				++c;
			}
		}

		constexpr void construct_from(CharType const* source) {
			auto len = Traits::length(source);
			initialize_storage(len);
			append_impl(source, len);
		}

		constexpr void construct_from(__BasicStrStorage const& source) {
			const auto len = source.size();
			initialize_storage(len);
			append_impl(source.begin(), source.size());
		}

		constexpr void destroy() {
			if(is_small_string()) {
				return;
			}

			buffer_free(m_storage.regular_string.buffer, m_storage.regular_string.buffer_size);
		}

		constexpr void initialize_storage(size_t length) {
			if(length > sizeof(__SmallStringData::string)) {
				m_storage.regular_string.buffer = nullptr;
				m_storage.regular_string.buffer_size = 0;
				m_storage.regular_string.count = 0;
			} else {
				m_storage.small_string.control.flag = true;
				m_storage.small_string.control.size = 0;
			}
		}

		constexpr void size_set(size_t new_size) {
			if(is_small_string()) {
				m_storage.small_string.control.size = new_size;
			} else {
				m_storage.regular_string.count = new_size;
			}
		}

		constexpr void resize_impl(size_t new_length, CharType def) {
			if(size() >= new_length) {
				size_set(new_length);
				return;
			}
			auto count = new_length - size();
			append_impl(def, count);
		}
	public:
		constexpr __BasicStrStorage()
		    : m_storage() {
			initialize_storage(0);
		}

		explicit constexpr __BasicStrStorage(CharType const* cstring) { construct_from(cstring); }

		constexpr __BasicStrStorage(__BasicStrStorage const& str) { construct_from(str); }

		constexpr __BasicStrStorage(__BasicStrStorage&& str) noexcept
		    : m_storage(gen::move(str.m_storage)) {}

		constexpr ~__BasicStrStorage() { destroy(); }

		constexpr CharType* begin() {
			if(is_small_string()) {
				return m_storage.small_string.string;
			} else {
				return m_storage.regular_string.buffer;
			}
		}

		constexpr CharType* end() {
			if(is_small_string()) {
				return m_storage.small_string.string + m_storage.small_string.control.size;
			} else {
				return m_storage.regular_string.buffer + m_storage.regular_string.count;
			}
		}

		constexpr CharType const* begin() const {
			if(is_small_string()) {
				return m_storage.small_string.string;
			} else {
				return m_storage.regular_string.buffer;
			}
		}

		constexpr CharType const* end() const {
			if(is_small_string()) {
				return m_storage.small_string.string + m_storage.small_string.control.size;
			} else {
				return m_storage.regular_string.buffer + m_storage.regular_string.count;
			}
		}

		constexpr CharType const* to_c_string() {
			//  Make sure there is at least one more byte in the buffer
			//  This might cause a migration from small string to heap buffer when a
			//  string of exactly the small string buffer length is stored.
			allocate_storage(1);
			//  We don't change the size - the null terminator is ephemeral, and will be
			//  overwritten by any string-mutating functions.

			//  Null terminate
			*(begin() + size()) = CharType {};
			return begin();
		}

		constexpr size_t size() const {
			if(is_small_string()) {
				return m_storage.small_string.control.size;
			} else {
				return m_storage.regular_string.count;
			}
		}

		constexpr size_t capacity() const {
			if(is_small_string()) {
				return sizeof(__SmallStringData::string);
			} else {
				return m_storage.regular_string.buffer_size;
			}
		}

		constexpr void clear() { size_set(0); }

		constexpr void resize(size_t size, CharType v) { resize_impl(size, v); }

		constexpr void reserve(size_t capacity) {
			if(capacity <= this->capacity()) {
				return;
			}
			const auto count = capacity - this->capacity();
			allocate_storage(count);
		}

		constexpr void append(CharType ch) { append_impl(ch); }

		constexpr void append(CharType const* cstr) { append_impl(cstr, Traits::length(cstr)); }

		constexpr void append(__BasicStrStorage const& str) { append_impl(str.begin(), str.size()); }

		constexpr __BasicStrStorage& operator=(__BasicStrStorage const& str) {
			m_storage = str.m_storage;
			return *this;
		}

		constexpr __BasicStrStorage& operator=(__BasicStrStorage&& str) noexcept {
			m_storage = gen::move(str.m_storage);
			return *this;
		}
	};

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	class __BasicStr {
		typedef char_traits<CharType> Traits;
		__BasicStrStorage<CharType, Alloc> m_storage;
	public:
		constexpr __BasicStr()
		    : m_storage() {}

		constexpr __BasicStr(__BasicStr const& str)
		    : m_storage(str.m_storage) {}

		constexpr __BasicStr(__BasicStr&& str) noexcept
		    : m_storage(gen::move(str.m_storage)) {}

		explicit constexpr __BasicStr(CharType const* cstring)
		    : m_storage(cstring) {}

		constexpr ~__BasicStr() = default;

		constexpr size_t size() const { return m_storage.size(); }
		constexpr bool empty() const { return m_storage.size() == 0; }

		constexpr void clear() { m_storage.clear(); }
		constexpr void resize(size_t count, CharType v = CharType {}) { m_storage.resize(count, v); }
		constexpr void reserve(size_t capacity) { m_storage.reserve(capacity); }
		constexpr size_t capacity() const { return m_storage.capacity(); }

		constexpr void append(CharType ch) { m_storage.append(ch); }
		constexpr void append(CharType const* cstring) { m_storage.append(cstring); }
		constexpr void append(__BasicStr const& str) { m_storage.append(str.m_storage); }

		constexpr __BasicStr& operator+=(CharType ch) {
			append(ch);
			return *this;
		}
		constexpr __BasicStr& operator+=(CharType const* cstring) {
			append(cstring);
			return *this;
		}
		constexpr __BasicStr& operator+=(__BasicStr const& other) {
			append(other);
			return *this;
		}

		constexpr __BasicStr& operator=(__BasicStr const& str) {
			if(this == &str) {
				return *this;
			}
			m_storage = __BasicStrStorage { str.m_storage };
			return *this;
		}
		constexpr __BasicStr& operator=(CharType const* cstring) {
			m_storage = __BasicStrStorage { cstring };
			return *this;
		}

		constexpr CharType* begin() { return m_storage.begin(); }
		constexpr CharType* end() { return m_storage.end(); }
		constexpr CharType const* begin() const { return m_storage.begin(); }
		constexpr CharType const* end() const { return m_storage.end(); }

		constexpr CharType const* data() { return m_storage.to_c_string(); }

		constexpr CharType& operator[](size_t index) { return *(begin() + index); }
		constexpr CharType const& operator[](size_t index) const { return *(begin() + index); }

		constexpr bool operator==(CharType const* cstring) const {
			auto len = Traits::length(cstring);
			if(len != size()) {
				return false;
			}
			return Traits::compare(begin(), cstring, len) == 0;
		}
		constexpr bool operator==(__BasicStr const& str) const {
			if(size() != str.size()) {
				return false;
			}
			return Traits::compare(begin(), str.begin(), size()) == 0;
		}
	};

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	constexpr bool operator==(__BasicStr<CharType, Alloc> const& lhs, __BasicStr<CharType, Alloc> const& rhs) {
		return lhs.operator==(rhs);
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	constexpr bool operator==(__BasicStr<CharType, Alloc> const& lhs, CharType const* rhs) {
		return lhs.operator==(rhs);
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	constexpr __BasicStr<CharType, Alloc> operator+(__BasicStr<CharType, Alloc> const& str,
	                                                __BasicStr<CharType, Alloc> const& str2) {
		__BasicStr<CharType, Alloc> tmp { str };
		tmp += str2;
		return tmp;
	}

	template<class CharType, template<typename> class Alloc = gen::Allocator>
	constexpr __BasicStr<CharType, Alloc> operator+(__BasicStr<CharType, Alloc> const& str, CharType const* str2) {
		__BasicStr<CharType, Alloc> tmp { str };
		tmp += str2;
		return tmp;
	}

	typedef __BasicStr<char> String;
}
