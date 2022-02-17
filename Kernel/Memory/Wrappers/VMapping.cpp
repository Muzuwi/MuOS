#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>
#include <Memory/Wrappers/VMapping.hpp>

VMapping::VMapping(void* addr, size_t size, int flags, int type)
    : m_pages()
    , m_addr(addr)
    , m_size(size)
    , m_flags(flags)
    , m_type(type) {}

SharedPtr<VMapping> VMapping::create(void* address, size_t size, uint32 flags, uint32_t type) {
	auto* vmapping = new(KHeap::instance().slab_alloc(sizeof(VMapping))) VMapping(address, size, flags, type);

	//  FIXME:  Fix unaligned sizes
	auto page_count = size / 0x1000;
	for(unsigned i = 0; i < page_count; ++i) {
		auto alloc = PMM::instance().allocate(0);
		assert(alloc.has_value());
		vmapping->m_pages.push_back(alloc.unwrap());
	}

	return SharedPtr<VMapping> { vmapping };
}

VMapping::~VMapping() {
	for(auto& alloc : m_pages) {
		PMM::instance().free(alloc);
	}
}

KOptional<PhysPtr<uint8>> VMapping::page_for(void* vaddr) const {
	if(vaddr < m_addr || vaddr >= (uint8_t*)m_addr + m_size) {
		return {};
	}

	auto* region_start = (uint8*)m_addr;
	for(auto& alloc : m_pages) {
		auto* region_end = region_start + alloc.size();
		if(vaddr >= region_start && vaddr < region_end) {
			auto offset = (uint8*)vaddr - (uint8*)region_start;
			auto addr = alloc.base().as<uint8>() + offset;
			return addr;
		}

		region_start = region_end;
	}

	return {};
}

bool VMapping::overlaps(VMapping const&) {
	//  TODO: Implement
	return false;
}
