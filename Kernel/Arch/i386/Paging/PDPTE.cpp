#include <string.h>
#include <Arch/i386/Paging.hpp>
#include <Kernel/Process/ProcMem.hpp>

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

PhysPtr<PDPT> PDPT::clone(ProcMem& pm) {
	auto page = pm.allocate_phys_kernel(0);
	assert(page.has_value());

	auto pdpt = page.unwrap().base().as<PDPT>();
	//  Clone flags, dirs should be overwritten
	memcpy(pdpt.get_mapped(), this, 0x1000);

	for(unsigned i = 0; i < 512; ++i) {
		//  Ignore huge pages
		if(m_entries[i].get(FlagPDPTE::HugePage))
			continue;

		//  Clone only present entries
		if(m_entries[i].get(FlagPDPTE::Present)) {
			auto pd = m_entries[i].directory()->clone(pm);
			pdpt->m_entries[i].set_directory(pd);
		}
	}

	return pdpt;
}
