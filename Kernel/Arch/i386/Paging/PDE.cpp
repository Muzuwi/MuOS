#include <string.h>
#include <Arch/i386/Paging.hpp>
#include <Kernel/Process/ProcMem.hpp>

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

PhysPtr<PD> PD::clone(ProcMem& pm) {
	auto page = pm.allocate_phys_kernel(0);
	assert(page.has_value());

	auto pd = page.unwrap().base().as<PD>();
	//  Clone flags, dirs should be overwritten
	memcpy(pd.get_mapped(), this, 0x1000);

	for(unsigned i = 0; i < 512; ++i) {
		//  Ignore large pages
		if(m_entries[i].get(FlagPDE::LargePage))
			continue;

		//  Clone only present entries
		if(m_entries[i].get(FlagPDE::Present)) {
			auto pt = m_entries[i].table()->clone(pm);
			pd->m_entries[i].set_table(pt);
		}
	}

	return pd;
}
