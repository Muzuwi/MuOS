#pragma once

#include <LibGeneric/List.hpp>
#include <Memory/Allocators/PAllocation.hpp>
#include <Memory/Wrappers/UserPtr.hpp>
#include <Structs/KOptional.hpp>
#include <SystemTypes.hpp>

enum VMappingFlags : uint32 {
	VM_KERNEL = 0x00000001,
	VM_READ = 0x00000002,
	VM_WRITE = 0x00000004,
	VM_EXEC = 0x00000008
};

enum VMappingType : uint32 {
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
	uint32 m_flags;
	uint32 m_type;

	VMapping(void* addr, size_t size, int flags, int type);
public:
	static SharedPtr<VMapping> create(void* address, size_t size, uint32 flags, uint32 type);

	VMapping(const VMapping&) = delete;

	VMapping(VMapping&&) = delete;

	~VMapping();

	int type() const { return m_type; }

	uint32 flags() const { return m_flags; }

	size_t size() const { return m_size; }

	List<PAllocation>& pages() { return m_pages; }

	List<PAllocation> const& pages() const { return m_pages; }

	void* addr() const { return m_addr; }

	void* end() const { return (void*)((uintptr_t)m_addr + m_size); }

	bool contains(void* vaddr) {
		return (uintptr_t)vaddr >= (uintptr_t)m_addr && (uintptr_t)vaddr < (uintptr_t)m_addr + m_size;
	}

	bool overlaps(VMapping const&);

	KOptional<PhysPtr<uint8>> page_for(void* vaddr) const;
};