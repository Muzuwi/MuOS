#include <Memory/UserPtr.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Debug/klogf.hpp>

KBox<const char> UserString::copy_to_kernel() {
	//  FIXME: Make sure a different thread cannot modify the address space while we're in here
	auto* user_ptr = (uint8_t*)m_ptr;
	auto& vmm = Thread::current()->parent()->vmm();

	constexpr unsigned str_max_size {128};
	unsigned size {0};
	while(size < str_max_size) {
		auto region = vmm.find_vmapping(user_ptr + size);
		if(!region.has_value()) {
			kerrorf("Thread[tid={}] - tried copying string from unaccessible memory", Thread::current()->tid());
			return KBox<const char>{};
		}

		auto page = region.unwrap()->page_for(user_ptr + size);
		if(!page.has_value()) {
			kerrorf("Thread[tid={}] - tried reading from unmapped memory", Thread::current()->tid());
			return KBox<const char>{};
		}

		auto byte = page.unwrap();

		if(*byte == 0x0) {
			break;
		}

		size++;
		if(size == str_max_size) {
			kerrorf("Thread[tid={}] - max string size exceeded during copy", Thread::current()->tid());
			return KBox<const char>{};
		}
	}

	auto* buf = (uint8_t*)KHeap::allocate(size+1);
	if(!buf) {
		return KBox<const char>{};
	}

	//  Copy from user memory to kernel buffer, byte by byte
	//  FIXME: Slowpath, most of the time the buffer will be within a single vmregion
	for(unsigned i = 0; i < size+1; ++i) {
		auto region = vmm.find_vmapping(user_ptr + i);
		assert(region.has_value());

		auto page = region.unwrap()->page_for(user_ptr + i);
		assert(page.has_value());

		auto phys_ptr = page.unwrap();
		buf[i] = *phys_ptr;
	}

	return KBox<const char>{buf, size + 1};
}
