#pragma once

class PageToken {
	friend class PMM;

	void* m_page;

	PageToken(void* addr)
	: m_page(addr) { }
public:
	void* address() const { return m_page; }
};