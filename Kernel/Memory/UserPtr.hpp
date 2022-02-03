#pragma once
#include <LibGeneric/Move.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <Memory/KBox.hpp>

template<class T>
class UserPtr {
	void* m_ptr;

	typedef gen::remove_pointer<T> type;
public:
	gen::SharedPtr<type> copy_to_kernel();
	bool copy_to_user(type*);
};

class UserString {
	void* m_ptr;
public:
	explicit UserString(void* user_ptr) noexcept
	: m_ptr(user_ptr) {}

	KBox<const char> copy_to_kernel();
};