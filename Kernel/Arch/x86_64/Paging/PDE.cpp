#include <string.h>
#include <Arch/x86_64/Paging.hpp>

bool PDE::get(FlagPDE flag) const {
	return m_data & static_cast<uint64_t>(flag);
}

void PDE::set(FlagPDE flag, bool value) {
	auto mask = static_cast<uint64_t>(flag);
	m_data &= ~mask;
	m_data |= (value ? mask : 0);
}

PhysPtr<PT> PDE::table() const {
	return PhysPtr<PT>((PT*)mask_address(m_data));
}

void PDE::set_table(PhysPtr<PT> page_table) {
	auto new_address = reinterpret_cast<uint64_t>(page_table.get());
	m_data &= ~_paging_address_mask;
	m_data |= new_address & _paging_address_mask;
}

const PDE& PD::operator[](void* address) const {
	return m_entries[index_pde(address)];
}

PDE& PD::operator[](void* address) {
	return m_entries[index_pde(address)];
}
