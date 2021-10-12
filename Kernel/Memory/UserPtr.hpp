#pragma once
#include <LibGeneric/Move.hpp>
#include <LibGeneric/SharedPtr.hpp>

template<class T>
class UserPtr {
	void* m_ptr;

	typedef gen::remove_pointer<T> type;
public:
	gen::SharedPtr<type> copy_to_kernel();
	bool copy_to_user(type*);
};

