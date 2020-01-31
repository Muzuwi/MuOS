#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __is_kernel_build__
#include <Kernel/Debug/kassert.hpp>
#endif

namespace gen {
    template<class CharType>
    class Basic_StringView {
        CharType* m_string;
        size_t m_size;
    public:
        Basic_StringView();
        Basic_StringView(CharType* string, size_t size);

        CharType& operator[](size_t n) const;
        CharType& at(size_t n) const;

        size_t size() const;
        bool empty() const;
    };

    template<class CharType>
    Basic_StringView<CharType>::Basic_StringView() {
        m_string = nullptr;
        m_size = 0;
    }

    template<class CharType>
    Basic_StringView<CharType>::Basic_StringView(CharType* string, size_t size) {
        m_string = string;
        m_size = size;
    }


    template<class CharType>
    CharType& Basic_StringView<CharType>::operator[](size_t n) const {
#ifdef USE_GEN_EXCEPTIONS
        //  TODO: Eventually allow exceptions in userland
#else
        assert(m_string);
        assert(n < m_size);
#endif
        return m_string[n];
    }

    template<class CharType>
    CharType &Basic_StringView<CharType>::at(size_t n) const {
        return this->operator[](n);
    }


    template<class CharType>
    size_t Basic_StringView<CharType>::size() const {
        return m_size;
    }

    template<class CharType>
    bool Basic_StringView<CharType>::empty() const {
        return m_size == 0;
    }

    typedef Basic_StringView<char> StringView;
}

