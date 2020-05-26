#pragma once
#include <LibGeneric/List.hpp>
#include <Kernel/Memory/PageToken.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Memory/PMM.hpp>

class VMapping {
private:
	friend class VMM;
	friend class Process;  //  FIXME:  I don't like this friendship

	gen::List<PageToken*> m_pages;

	void* m_addr;
	size_t m_size;
	int m_flags;
	int m_type;

	VMapping(void* addr, size_t size, int flags, int type)
	: m_pages(), m_addr(addr), m_size(size), m_flags(flags), m_type(type) {
		if(size % 4096 != 0)
			kerrorf("[VMapping] Creating VMapping with unaligned size (%x)!\n", size);
		if((uint64_t)addr % 4096 != 0)
			kerrorf("[VMapping] Creating VMapping with unaligned addr (%x)!\n", addr);

		for(unsigned i = 0; i < size / 4096; ++i)
			m_pages.push_back(PMM::allocate_page_user());

		VMM::notify_create_VMapping(*this);
	}

	VMapping(const VMapping&) = delete;
	VMapping(VMapping&&) = delete;
public:
	~VMapping() {
		VMM::notify_free_VMapping(*this);
	}

	int type() const {
		return m_type;
	}

	int flags() const {
		return m_flags;
	}

	gen::List<PageToken*>& pages() {
		return m_pages;
	}

	const gen::List<PageToken*>& pages() const {
		return m_pages;
	}

	void* addr() const {
		return m_addr;
	}

};