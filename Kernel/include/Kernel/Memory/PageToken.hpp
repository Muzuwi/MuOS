#pragma once
#include <Kernel/Memory/PMM.hpp>

class PageToken {
	friend class PMM;

	void* m_page;

	PageToken(void* addr)
	: m_page(addr) { }
public:
	void* address() const { return m_page; }

	~PageToken() {
		PMM::free_page_from_token(this);
	}
};