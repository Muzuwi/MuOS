#include <Core/Mem/Heap.hpp>
#include <Memory/Wrappers/UserPtr.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>

template<class T>
gen::SharedPtr<typename UserPtr<T>::type> UserPtr<T>::copy_to_kernel() {
	auto* user_ptr = (uint8*)m_ptr;
	auto& vmm = Thread::current()->parent()->vmm();
	auto lock = vmm.acquire_vm_lock();
	auto size = sizeof(T);

	//  Verify whether the entire structure is mapped in the process
	//  FIXME: Discarding find_vmregion results to later redo them again
	for(unsigned i = 0; i < size; ++i) {
		auto region = vmm.find_vmapping(user_ptr + i);
		if(!region.has_value()) {
			return gen::SharedPtr<type> { nullptr };
		}
	}

	auto* buf = (uint8*)core::mem::hmalloc(size);
	if(!buf) {
		return gen::SharedPtr<type> { nullptr };
	}

	//  Copy from user memory to kernel buffer, byte by byte
	//  FIXME: Slowpath, most of the time the buffer will be within a single vmregion
	for(unsigned i = 0; i < size; ++i) {
		auto region = vmm.find_vmapping(user_ptr + i);
		ENSURE(region.has_value());

		auto page = region.unwrap()->page_for(user_ptr + i);
		ENSURE(page.has_value());

		auto phys_ptr = page.unwrap();
		buf[i] = *phys_ptr;
	}

	return gen::SharedPtr<type> { reinterpret_cast<type*>(buf) };
}

template<class T>
bool UserPtr<T>::copy_to_user(type*) {
	//  FIXME: TODO
	return false;
}
