#include <Kernel/Memory/UserPtr.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/ProcMem.hpp>
#define S(a) (uintptr_t)a>>32u, (uintptr_t)a&0xffffffffu

template<>
gen::SharedPtr<typename UserPtr<const char>::type> UserPtr<const char>::copy_to_kernel() {
	//  FIXME: Make sure a different thread cannot modify the address space while we're in here
	auto* user_ptr = (uint8_t*)m_ptr;
	auto& mem = *(Process::current()->memory());

	constexpr unsigned str_max_size {128};
	unsigned size {0};
	while(size < str_max_size) {
		auto region = mem.find_vmregion(user_ptr + size);
		if(!region.has_value()) {
			kerrorf("Process[pid=%i] - tried copying string from unaccessible memory", Process::current()->pid());
			return gen::SharedPtr<type>{nullptr};
		}

		auto page = region.unwrap()->mapping().page_for(user_ptr + size);
		if(!page.has_value()) {
			kerrorf("Process[pid=%i] - tried reading from unmapped memory", Process::current()->pid());
			return gen::SharedPtr<type>{nullptr};
		}

		auto byte = page.unwrap();

		if(*byte == 0x0) {
			break;
		}

		size++;
		if(size == str_max_size) {
			kerrorf("Process[pid=%i] - max string size exceeded during copy", Process::current()->pid());
			return gen::SharedPtr<type>{nullptr};
		}
	}

	auto* buf = (uint8_t*)KHeap::allocate(size+1);
	if(!buf) {
		return gen::SharedPtr<type>{nullptr};
	}

	//  Copy from user memory to kernel buffer, byte by byte
	//  FIXME: Slowpath, most of the time the buffer will be within a single vmregion
	for(unsigned i = 0; i < size+1; ++i) {
		auto region = mem.find_vmregion(user_ptr + i);
		assert(region.has_value());

		auto page = region.unwrap()->mapping().page_for(user_ptr + i);
		assert(page.has_value());

		auto phys_ptr = page.unwrap();
		buf[i] = *phys_ptr;
	}

	return gen::SharedPtr<type>{reinterpret_cast<type*>(buf)};
}
