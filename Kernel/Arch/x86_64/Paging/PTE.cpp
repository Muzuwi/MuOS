#include <Arch/x86_64/Paging.hpp>
#include <string.h>

bool PTE::get(FlagPTE flag) const {
	return m_data & static_cast<uint64_t>(flag);
}

void PTE::set(FlagPTE flag, bool value) {
	auto mask = static_cast<uint64_t>(flag);
	m_data &= ~mask;
	m_data |= (value ? mask : 0);
}

void PTE::set_page(PhysAddr page) {
	auto new_address = reinterpret_cast<uint64_t>(page.get());
	m_data &= ~_paging_address_mask;
	m_data |= new_address & _paging_address_mask;
}

const PTE& PT::operator[](void* address) const {
	return m_entries[index_pte(address)];
}

PTE& PT::operator[](void* address) {
	return m_entries[index_pte(address)];
}
