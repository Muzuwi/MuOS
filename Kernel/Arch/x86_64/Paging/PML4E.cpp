#include <string.h>
#include <Arch/x86_64/Paging.hpp>

bool PML4E::get(FlagPML4E flag) const {
	return m_data & static_cast<uint64_t>(flag);
}

void PML4E::set(FlagPML4E flag, bool value) {
	auto mask = static_cast<uint64_t>(flag);
	m_data &= ~mask;
	m_data |= (value ? mask : 0);
}

PhysPtr<PDPT> PML4E::directory() {
	return PhysPtr<PDPT>((PDPT*)mask_address(m_data));
}

void PML4E::set_directory(PhysPtr<PDPT> page_directory_ptr_table) {
	auto new_address = reinterpret_cast<uint64_t>(page_directory_ptr_table.get());
	m_data &= ~_paging_address_mask;
	m_data |= new_address & _paging_address_mask;
}

const PML4E& PML4::operator[](void* address) const {
	return m_entries[index_pml4e(address)];
}

PML4E& PML4::operator[](void* address) {
	return m_entries[index_pml4e(address)];
}
