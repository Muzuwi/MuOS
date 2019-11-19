#pragma once
#include <stdint.h>
#include <kernel/SystemTypes.hpp>

#define MiB 0x100000
#define KMALLOC_POOL_SIZE  (4 * MiB) 
#define KMALLOC_CHUNK 	2
#define KMALLOC_ARR_COUNT ((KMALLOC_POOL_SIZE / KMALLOC_CHUNK) / (sizeof(chunk_t)*8))

typedef uint32_t chunk_t; 

#define SANITIZE_KMALLOC_MEM_RANGE


void* operator new(size_t);
void* operator new[](size_t);
void operator delete(void*, size_t);
void operator delete[](void*, size_t);


class KMalloc {
	KMalloc();
public:
	static KMalloc& get();
	
	void init();
	uint64_t getPoolSize();
	void* kmalloc_alloc(size_t);
	void kmalloc_free(void*);
	//  Other alloc fun stuff
};
