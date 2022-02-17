#pragma once
#include <Arch/x86_64/Paging/PDE.hpp>
#include <Arch/x86_64/Paging/PDPTE.hpp>
#include <Arch/x86_64/Paging/PML4E.hpp>
#include <Arch/x86_64/Paging/PTE.hpp>

static inline uint64_t index_pml4e(void* value) {
	return (reinterpret_cast<uint64_t>(value) >> 39u) & (0x1ffu);
}

static inline uint64_t index_pdpte(void* value) {
	return (reinterpret_cast<uint64_t>(value) >> 30u) & (0x1ffu);
}

static inline uint64_t index_pde(void* value) {
	return (reinterpret_cast<uint64_t>(value) >> 21u) & (0x1ffu);
}

static inline uint64_t index_pte(void* value) {
	return (reinterpret_cast<uint64_t>(value) >> 12u) & (0x1ffu);
}

static inline constexpr uint64_t _paging_address_mask = 0x000ffffffffff000u;

static inline uint64_t mask_address(uint64_t data) {
	//	return (data & _paging_address_mask) >> 12u;
	return (data & _paging_address_mask);
}
