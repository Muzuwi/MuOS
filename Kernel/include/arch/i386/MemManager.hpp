#pragma once
#include <stdint.h>
#include <stddef.h>

//  TODO: This will not be necessary once i get page frame allocation going
#define MEMMANAGER_MAX_MEM_RANGES 32

struct mem_range_t {
	uint64_t m_start, m_end;
	mem_range_t(uint64_t start, uint64_t end) {
		m_start = start;
		m_end = end;
	}
	mem_range_t(){
		m_start = 0;
		m_end = 0;
	}
};

class MemManager final {
	mem_range_t m_free_mem_ranges[MEMMANAGER_MAX_MEM_RANGES];
	mem_range_t m_kernel_memory_ranges[MEMMANAGER_MAX_MEM_RANGES];
	mem_range_t m_user_memory_ranges[MEMMANAGER_MAX_MEM_RANGES];

	unsigned int m_free_mem_ranges_count = 0;
	static MemManager* instance;
public:
	MemManager();
	static MemManager& get();
	void parse_multiboot_mmap(uintptr_t*);
	uint64_t get_free();
	void* allocate_kernel(size_t);
};