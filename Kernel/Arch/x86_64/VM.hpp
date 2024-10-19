#pragma once
#include <Arch/x86_64/LinkscriptSyms.hpp>
#include <LibGeneric/BitFlags.hpp>
#include <SystemTypes.hpp>

/* Base address of the identity region in virtual memory */
#define KERNEL_VM_IDENTITY_BASE (reinterpret_cast<void*>(&_ukernel_identity_start))
/* Length of the identity region in virtual memory */
#define KERNEL_VM_IDENTITY_LEN \
	(reinterpret_cast<uintptr_t>(&_ukernel_heap_start) - reinterpret_cast<void*>(&_ukernel_identity_start))

/* Start address of the VM region that will be shared across ALL processes */
#define KERNEL_VM_SHARED_START (reinterpret_cast<void*>(&_ukernel_shared_start))
/* End address of the shared VM region */
#define KERNEL_VM_SHARED_END (reinterpret_cast<void*>(&_ukernel_shared_end))

/* Start address of the kernel VM region */
#define KERNEL_VM_START (reinterpret_cast<void*>(&_ukernel_virtual_start))

namespace arch {
	static inline constexpr uint64 PAGING_ADDRESS_MASK = 0x000ffffffffff000u;

	static inline uint64 index_pml4e(void* value) {
		return (reinterpret_cast<uint64>(value) >> 39u) & (0x1ffu);
	}

	static inline uint64 index_pdpte(void* value) {
		return (reinterpret_cast<uint64>(value) >> 30u) & (0x1ffu);
	}

	static inline uint64 index_pde(void* value) {
		return (reinterpret_cast<uint64>(value) >> 21u) & (0x1ffu);
	}

	static inline uint64 index_pte(void* value) {
		return (reinterpret_cast<uint64>(value) >> 12u) & (0x1ffu);
	}

	static inline void* mask_address(uint64 paging_entry) {
		return reinterpret_cast<void*>(paging_entry & PAGING_ADDRESS_MASK);
	}

	static inline bool is_page_aligned(void* ptr) {
		return (reinterpret_cast<uintptr_t>(ptr) & 0xFFF) == 0;
	}

	static inline bool is_large_page_aligned(void* ptr) {
		return (reinterpret_cast<uintptr_t>(ptr) & 0x1FFFFF) == 0;
	}

	static inline bool is_huge_page_aligned(void* ptr) {
		return (reinterpret_cast<uintptr_t>(ptr) & 0x1FFFFFFF) == 0;
	}

	//  Common flags for all entry types
	enum class EntryFlags : uint64 {
		ExecuteDisable = 1ul << 63u,
		Accessed = 1 << 5u,
		Cache = 1 << 4u,
		WriteThrough = 1 << 3u,
		User = 1 << 2u,
		RW = 1 << 1u,
		Present = 1 << 0u,
	};
	DEFINE_ENUM_BITFLAG_OPS(EntryFlags);

	//  PDPTE-specific flags
	enum class FlagPDPTE : uint64 {
		HugePage = 1 << 7u,
	};
	DEFINE_ENUM_BITFLAG_OPS(FlagPDPTE);

	//  PDE-specific flags
	enum class FlagPDE : uint64 {
		LargePage = 1 << 7u,
	};
	DEFINE_ENUM_BITFLAG_OPS(FlagPDE);

	//  PTE-specific flags
	enum class FlagPTE : uint64 {
		Global = 1 << 8u,
		PAT = 1 << 7u,
		Dirty = 1 << 6u,
	};
	DEFINE_ENUM_BITFLAG_OPS(FlagPTE);

	//  Generic page table entry struct, common for all entry types
	struct PagingEntry {
		uint64 data;

		//  Get the state of a given flag
		template<typename T>
		constexpr bool get(T flag) {
			return static_cast<uint64>(flag) & data;
		}

		//  Clear all flag bits of the entry and set them to the given value
		template<typename T>
		constexpr void reset(T flags) {
			data &= ~static_cast<uint64>(flags);
			data |= static_cast<uint64>(flags);
		}

		//  Set the value of a given flag
		template<typename T>
		constexpr void set(T flag, bool value) {
			data &= ~static_cast<uint64>(flag);
			if(value) {
				data |= static_cast<uint64>(flag);
			}
		}

		//  Set the address stored in the entry
		inline void setaddr(void* addr) {
			data &= ~PAGING_ADDRESS_MASK;
			data |= reinterpret_cast<uint64>(addr);
		}

		//  Get the address stored in the entry
		inline void* getaddr() { return reinterpret_cast<void*>(data & PAGING_ADDRESS_MASK); }
	} __attribute__((packed));

	//  Generic paging table struct, common for all table types
	struct PagingTable {
		uint64 data[512];

		PagingEntry* get_pml4e(void* vptr) { return reinterpret_cast<PagingEntry*>(data + index_pml4e(vptr)); }
		PagingEntry* get_pdpte(void* vptr) { return reinterpret_cast<PagingEntry*>(data + index_pdpte(vptr)); }
		PagingEntry* get_pde(void* vptr) { return reinterpret_cast<PagingEntry*>(data + index_pde(vptr)); }
		PagingEntry* get_pte(void* vptr) { return reinterpret_cast<PagingEntry*>(data + index_pte(vptr)); }
	} __attribute__((packed));
}
