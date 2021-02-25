#pragma once
#include <LibGeneric/List.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Memory/PAllocation.hpp>

enum VMappingFlags : uint32_t {
	VM_KERNEL = 0x00000001,
	VM_READ = 0x00000002,
	VM_WRITE = 0x00000004,
	VM_EXEC = 0x00000008
};

enum VMappingType : uint32_t {
	MAP_SHARED = 0x00000000,
	MAP_PRIVATE = 0x00000001
};

class Process;

class VMapping {
private:
	friend class VMM;

	template<class T>
	using List = gen::List<T>;
	template<class T>
	using SharedPtr = gen::SharedPtr<T>;

	List<PAllocation> m_pages;

	void* m_addr;
	size_t m_size;
	uint32_t m_flags;
	uint32_t m_type;

	VMapping(void* addr, size_t size, int flags, int type)
	: m_pages(), m_addr(addr), m_size(size), m_flags(flags), m_type(type) {
		if(size % 4096 != 0)
			kerrorf("[VMapping] Creating VMapping with unaligned size (%x)!\n", size);
		if((uint64_t)addr % 4096 != 0)
			kerrorf("[VMapping] Creating VMapping with unaligned addr (%x)!\n", addr);
	}
public:
	static SharedPtr<VMapping> create(void* address, size_t size, uint32_t flags, uint32_t type) {
		auto* vmapping = new (KHeap::allocate(sizeof(VMapping))) VMapping(address, size, flags, type);

		//  FIXME:  Fix unaligned sizes
		auto page_count = size / 0x1000;
		for(unsigned i = 0; i < page_count; ++i) {
			auto alloc = PMM::allocate(0);
			assert(alloc.has_value());
			vmapping->m_pages.push_back(alloc.unwrap());
		}

		return SharedPtr<VMapping>{vmapping};
	}

	VMapping(const VMapping&) = delete;
	VMapping(VMapping&&) = delete;
	~VMapping() {
		for(auto& alloc : m_pages)
			PMM::free_allocation(alloc);
	}

	int type() const {
		return m_type;
	}

	uint32_t flags() const {
		return m_flags;
	}

	size_t size() const {
		return m_size;
	}

	List<PAllocation>& pages() {
		return m_pages;
	}

	List<PAllocation> const& pages() const {
		return m_pages;
	}

	void* addr() const {
		return m_addr;
	}

	void map(Process* process) const {
		VMM::vmapping_map(process, *this);
	}

	void unmap(Process* process) const {
		VMM::vmapping_unmap(process, *this);
	}
};