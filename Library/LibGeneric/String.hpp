#pragma once
#include <stdint.h>
#include <stddef.h>

#include <LibGeneric/TypeTraits/CharTraits.hpp>
#include <LibGeneric/Basic_StringView.hpp>

#ifdef __is_kernel_build__
#include <Kernel/Debug/kassert.hpp>
#endif

namespace gen {
    template<class CharType>
    class Basic_String {
        typedef char_traits<CharType> Traits;

        CharType* m_string;
        size_t m_size;
        size_t m_buffer_size;

        void from_basic_string(const Basic_String&);
        void from_chartype(const CharType*);

    public:
        Basic_String();
        Basic_String(const Basic_String&);
        Basic_String(const CharType*);
        ~Basic_String();

        CharType& at(size_t) const;
        CharType& operator[](size_t) const;

        Basic_String& operator+=(const Basic_String&);
        Basic_String& operator+=(const CharType*);
        Basic_String& operator+=(CharType);

        Basic_String& operator=(const Basic_String&);
        Basic_String& operator=(const CharType*);

        size_t length() const;
        size_t size() const;
        bool empty() const;

        size_t find(CharType ch) const;
        size_t find(const Basic_String&) const;

        Basic_StringView<CharType> substr(size_t start, size_t n);

        CharType* chars();

        void clear();
    };

    template<class CharType>
    Basic_String<CharType>::Basic_String() {
        m_string = nullptr;
        m_size = 0;
        m_buffer_size = 0;
    }

    template<class CharType>
    Basic_String<CharType>::Basic_String(const Basic_String& string) {
        this->from_basic_string(string);
    }

    template<class CharType>
    Basic_String<CharType>::Basic_String(const CharType* c_string) {
        this->from_chartype(c_string);
    }

    template<class CharType>
    Basic_String<CharType>::~Basic_String() {
        this->clear();
    }

    template<class CharType>
    void Basic_String<CharType>::from_basic_string(const Basic_String &string) {
        m_size = string.m_size;
        m_buffer_size = static_cast<size_t>(m_size*1.5f + 0.5f);
        m_string = nullptr;

        if(m_size == 0) return;

        m_string = new CharType[m_buffer_size];
        Traits::copy(m_string, string.m_string, string.m_size);
    }

    template<class CharType>
    void Basic_String<CharType>::from_chartype(const CharType *c_str) {
        m_size = Traits::length(c_str);
        m_buffer_size = static_cast<size_t>(m_size*1.5f + 0.5f);
        m_string = nullptr;

        if(m_size == 0) return;

        m_string = new CharType[m_buffer_size];
        Traits::copy(m_string, c_str, m_size);
    }

    template<class CharType>
    void Basic_String<CharType>::clear() {
        delete[] m_string;
        m_string = nullptr;
        m_size = 0;
        m_buffer_size = 0;
    }



    template<class CharType>
    size_t Basic_String<CharType>::length() const {
        return this->size();
    }

    template<class CharType>
    size_t Basic_String<CharType>::size() const {
        return m_size;
    }

    template<class CharType>
    bool Basic_String<CharType>::empty() const {
        return m_size == 0;
    }

    template<class CharType>
    CharType& Basic_String<CharType>::operator[](size_t n) const {
#ifdef USE_GEN_EXCEPTIONS
        //  TODO: Eventually allow exceptions in userland
#else
        assert(n < m_size);
#endif
        return m_string[n];
    }

    template<class CharType>
    CharType& Basic_String<CharType>::at(size_t n) const {
        return this->operator[](n);
    }

    template<class CharType>
    Basic_StringView<CharType> Basic_String<CharType>::substr(size_t start, size_t n) {
#ifdef USE_GEN_EXCEPTIONS
        //  TODO: Eventually allow exceptions in userland
#else
        assert(n < m_size);
        assert(start + n*sizeof(CharType) < m_size);
#endif
        return Basic_StringView<CharType>(m_string + sizeof(CharType)*start, n);
    }

    /*
     *      Overloaded operators for strings
     */

    template<class CharType>
    Basic_String<CharType>& Basic_String<CharType>::operator+=(const Basic_String& string) {
        if(string.m_size == 0) return *this;

        size_t required_buf_size = m_size + string.m_buffer_size;

        if(required_buf_size < m_buffer_size - m_size) {
            Traits::copy(m_string, string.m_string, string.m_size, m_size);
            m_size += string.m_size;
        } else {
            auto new_buf_size = static_cast<size_t>(required_buf_size*1.5f + 0.5f);
            auto new_buffer = new CharType[new_buf_size];

            Traits::copy(new_buffer, m_string, m_size);
            Traits::copy(new_buffer, string.m_string, string.m_size, m_size);

            delete[] m_string;

            m_string = new_buffer;
            m_buffer_size = new_buf_size;
            m_size += string.m_size;
        }

        return *this;
    }

    template<class CharType>
    Basic_String<CharType>& Basic_String<CharType>::operator+=(const CharType *c_string) {
        size_t len = Traits::length(c_string);
        if(len == 0) return *this;

        size_t required_buf_size = m_size + len;

        if(required_buf_size < m_buffer_size) {
            Traits::copy(m_string, c_string, len, m_size);
            m_size += len;
        } else {
            auto new_buf_size = static_cast<size_t>(required_buf_size*1.5f + 0.5f);
            auto new_buffer = new CharType[new_buf_size];

            Traits::copy(new_buffer, m_string, m_size);
            Traits::copy(new_buffer, c_string, len, m_size);

            delete[] m_string;

            m_string = new_buffer;
            m_size += len;
            m_buffer_size = new_buf_size;
        }

        return *this;
    }

    template<class CharType>
    Basic_String<CharType>& Basic_String<CharType>::operator+=(CharType character) {
        if(m_buffer_size - m_size > 1) {
            m_string[m_size++] = character;
        } else {
            size_t required_buf_size = m_size + 1;
            auto new_buf_size = static_cast<size_t>(required_buf_size*1.5f + 0.5f);
            auto new_buffer = new CharType[new_buf_size];

            Traits::copy(new_buffer, m_string, m_size);

            new_buffer[m_size] = character;

            delete[] m_string;
            m_string = new_buffer;
            m_size++;
            m_buffer_size = new_buf_size;
        }

        return *this;
    }

    template<class CharType>
    Basic_String<CharType> operator+(const Basic_String<CharType> &A, const Basic_String<CharType> &B){
        Basic_String<CharType> result(A);
        result += B;
        return result;
    }

    template<class CharType>
    Basic_String<CharType> operator+(const Basic_String<CharType> &A, const CharType *B){
        Basic_String<CharType> result(A);
        result += B;
        return result;
    }

    template<class CharType>
    Basic_String<CharType> operator+(const Basic_String<CharType> &A, const CharType &B){
        Basic_String<CharType> result(A);
        result += B;
        return result;
    }

    template<class CharType>
    Basic_String<CharType>& Basic_String<CharType>::operator=(const Basic_String<CharType> &str) {
        if(this == &str)
            return *this;
        this->clear();
        this->from_basic_string(str);
        return *this;
    }

    template<class CharType>
    Basic_String<CharType>& Basic_String<CharType>::operator=(const CharType *c_str) {
        this->clear();
        this->from_chartype(c_str);
        return *this;
    }

    template<class CharType>
    size_t Basic_String<CharType>::find(CharType ch) const {
        Basic_String<CharType> temp;
        temp += ch;

        return Traits::find(this->m_string, this->m_size, temp.m_string, temp.m_size);
    }

    template<class CharType>
    size_t Basic_String<CharType>::find(const Basic_String &str) const {
        return Traits::find(this->m_string, this->m_size, str.m_string, str.m_size);
    }

    template<class CharType>
    CharType* Basic_String<CharType>::chars() {
        return m_string;
    }

    typedef Basic_String<char> String;
}

