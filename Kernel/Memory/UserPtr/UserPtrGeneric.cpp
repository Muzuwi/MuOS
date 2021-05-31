#include <Kernel/Memory/UserPtr.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/ProcMem.hpp>

template<class T>
gen::SharedPtr<typename UserPtr<T>::type> UserPtr<T>::copy_to_kernel() {
	//  FIXME: Make sure a different thread cannot modify the address space while we're in here
	auto* user_ptr = (uint8_t*)m_ptr;
	auto& mem = *(Process::current()->memory());
	auto size = sizeof(T);

	//  Verify whether the entire structure is mapped in the process
	//  FIXME: Discarding find_vmregion results to later redo them again
	for(unsigned i = 0; i < size; ++i) {
		auto region = mem.find_vmregion(user_ptr + i);
		if(!region.has_value()) {
			kerrorf("Process[pid=%i] - tried copying from unaccessible memory", Process::current()->pid());
			return gen::SharedPtr<type>{nullptr};
		}
	}

	auto* buf = (uint8_t*)KHeap::allocate(size);
	if(!buf) {
		return gen::SharedPtr<type>{nullptr};
	}

	//  Copy from user memory to kernel buffer, byte by byte
	//  FIXME: Slowpath, most of the time the buffer will be within a single vmregion
	for(unsigned i = 0; i < size; ++i) {
		auto region = mem.find_vmregion(user_ptr + i);
		assert(region.has_value());

		auto page = region.unwrap()->mapping().page_for(user_ptr + i);
		assert(page.has_value());

		auto phys_ptr = page.unwrap();
		buf[i] = *phys_ptr;
	}

	return gen::SharedPtr<type>{reinterpret_cast<type*>(buf)};
}

template<class T>
bool UserPtr<T>::copy_to_user(type* kernel_ptr) {
	//  FIXME: TODO
	return false;
}

template class UserPtr<const char>;