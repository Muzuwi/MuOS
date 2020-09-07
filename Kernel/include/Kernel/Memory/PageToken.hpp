#pragma once
#include <Kernel/Memory/PMM.hpp>

class PageToken {
	//  Note: Implemented in the PMM
	friend gen::SharedPtr<PageToken> _allocate_page_internal(gen::List<PRegion*>&);

	void* m_page;

	PageToken(void* addr)
	: m_page(addr) { }
public:
	void* address() const { return m_page; }

	~PageToken() {
		PMM::free_page_from_token(this);
	}
};