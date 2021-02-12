#include <Arch/i386/Paging.hpp>

bool PDPTE::get(FlagPDPTE flag) const {
	return m_data & static_cast<uint64_t>(flag);
}

void PDPTE::set(FlagPDPTE flag, bool value) {
	auto mask = static_cast<uint64_t>(flag);
	m_data &= ~mask;
	m_data |= (value ? mask : 0);
}

PhysPtr<PD> PDPTE::directory() {
	return PhysPtr<PD>((PD*)mask_address(m_data));
}

void PDPTE::set_directory(PhysPtr<PD> page_directory) {
	auto new_address = reinterpret_cast<uint64_t>(page_directory.get());
	m_data &= ~_paging_address_mask;
	m_data |= new_address & _paging_address_mask;
}

const PDPTE& PDPT::operator[](void* address) const {
	return m_entries[index_pdpte(address)];
}

PDPTE& PDPT::operator[](void* address) {
	return m_entries[index_pdpte(address)];
}
