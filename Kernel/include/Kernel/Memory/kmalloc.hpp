#pragma once
#include <stdint.h>
#include <Kernel/SystemTypes.hpp>

#define MiB 0x100000
#define KMALLOC_POOL_SIZE  (4 * MiB) 
#define KMALLOC_CHUNK 	2
#define KMALLOC_ARR_COUNT ((KMALLOC_POOL_SIZE / KMALLOC_CHUNK) / (sizeof(chunk_t)*8))

typedef uint32_t chunk_t; 

void* operator new(size_t);
void* operator new[](size_t);
void operator delete(void*);
void operator delete(void*, size_t);
void operator delete[](void*, size_t);
void operator delete[](void*);


class KMalloc {
	KMalloc();
	uint64_t m_total_allocations;
	uint64_t m_total_frees;
	uint64_t m_current_allocations;

	mem_range_t m_kmalloc_mem_range;

	void mark_range(size_t, size_t, size_t, size_t, bool);
	bool is_kmalloc_memory(void*);
public:
	static KMalloc& get();
	
	void init();
	uint64_t getPoolSize();
	void* kmalloc_alloc(size_t);
	void kmalloc_free(void*);
	uint64_t getCurrentAllocations();
	uint64_t getTotalAllocations();
	uint64_t getTotalFrees();
};
