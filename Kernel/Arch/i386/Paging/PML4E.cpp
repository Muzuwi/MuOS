#include <string.h>
#include <Arch/i386/Paging.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Process/ProcMem.hpp>

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

PhysPtr<PML4> PML4::clone(ProcMem& pm) {
	auto page = pm.allocate_phys_kernel(0);
	assert(page.has_value());

	auto pml4 = page.unwrap().base().as<PML4>();
	//  Clone flags, dirs should be overwritten
	memcpy(pml4.get_mapped(), this, 0x1000);

	for(unsigned i = 0; i < 512; ++i) {
		//  Kernel shared memory should be copied as-is
		if(i >= index_pml4e(&_ukernel_shared_start) && i <= index_pml4e(&_ukernel_shared_end)) {
			pml4->m_entries[i] = m_entries[i];
			continue;
		}

		//  Clone only present entries
		if(m_entries[i].get(FlagPML4E::Present)) {
			auto pdpt = m_entries[i].directory()->clone(pm);
			pml4->m_entries[i].set_directory(pdpt);
		}
	}

	return pml4;
}
