#include <kernel/kmalloc.hpp>
#include <kernel/VirtualMemManager.hpp>
#include <kernel/kdebugf.hpp>
#include <kernel/kpanic.hpp>
#include <string.h>

mem_range_t kmalloc_mem_range;

chunk_t mem_allocations[KMALLOC_ARR_COUNT]{

};

KMalloc::KMalloc() {

}


KMalloc& KMalloc::get() {
	static KMalloc km;
	return km;
}


void KMalloc::init() {
	kmalloc_mem_range = mem_range_t(0, 0);

	// memset((void*)mem_allocations, 0, sizeof(mem_allocations)*sizeof(uint32_t));
	//  TODO: memset is horribly overengineered, replace this eventually
	for(size_t i = 0; i < KMALLOC_ARR_COUNT; i++) mem_allocations[i] = 0;

	kmalloc_mem_range = VirtualMemManager::get().bootstrap(KMALLOC_POOL_SIZE);

#ifdef SANITIZE_KMALLOC_MEM_RANGE
	for(size_t i = kmalloc_mem_range.m_start; i < kmalloc_mem_range.m_end; i += 4) {
		*(uint32_t*)i = 0;
	}
#endif
}

/*
	This is just a helper to easily mark allocations that span 
	multiple rray entries as used
*/
void mark_range(size_t start_index, size_t end_index, size_t start_chunk, size_t end_chunk) {
	// kdebugf("[KMalloc] marking chunks %x:%x spanning index %x:%x as alloc'd\n", start_chunk, end_chunk, start_index, end_index);

	for(size_t i = start_index; i <= end_index; i++) {
		size_t upper_bound = sizeof(chunk_t)*32;
		size_t start = 0;

		if(i == start_index) start = start_chunk;
		if(i == end_index) upper_bound = end_chunk+1;
		if(start_index == end_index) {
			start = start_chunk;
			upper_bound = end_chunk+1;
		}

		for(size_t chunk = start; chunk < upper_bound; chunk++) mem_allocations[i] |= (1 << chunk);

		if(start_index == end_index) return;
	}

}

void* KMalloc::kmalloc_alloc(size_t size) {
	//  We're using a structure at the beginning of the 
	//  allocated memory to make freeing memory easier
	size_t proper_size = size + sizeof(allocation_t);
	size_t chunks = proper_size / KMALLOC_CHUNK;
	if(proper_size % KMALLOC_CHUNK) chunks++;

	size_t available = 0;
	size_t start = 0, start_index = 0;

	for(size_t i = 0; i < KMALLOC_ARR_COUNT; i++){
		if(mem_allocations[i] == 0xffffffff) {
			available = 0;
			start = 0;
			start_index = i+1;
			continue;
		}

		for(size_t current = 0; current < sizeof(chunk_t)*8; current++){
			if(!((1 << current) & mem_allocations[i])) {
				available++; 
			}else {
				available = 0;
				start = current+1;
				start_index = i;
				continue;
			}

			if(available >= chunks){
				// kdebugf("[KMalloc] available %i\n", available);
				mark_range(start_index, i, start, current);

				uint32_t mem_offset = i*KMALLOC_CHUNK*32 + (start)*KMALLOC_CHUNK;
				uint32_t* memory_begin = (uint32_t*)(kmalloc_mem_range.m_start + mem_offset);
				// kdebugf("[KMalloc] found at %x, chunks %i-%i, offset %x\n", memory_begin, (int)start, (int)(start+chunks-1), (uint32_t)mem_offset);

				*(allocation_t*)memory_begin = (allocation_t)size;

				void* allocated_begin = (void*)((uint32_t)memory_begin+sizeof(allocation_t));

				kdebugf("[KMalloc] allocated %i bytes to %x\n", size, (uint32_t)allocated_begin);

				#ifdef KMALLOC_SANITIZER

				#endif

				return allocated_begin;
			}

		}

		if(available == 0) {
			start_index = i + 1;
		}

	}

	kerrorf("[KMalloc] cannot find %i free chunks for %i allocation (out of memory?)\n", chunks, size);
	kpanic();

}

void KMalloc::kmalloc_free(void* pointer) {

}



void* operator new(size_t size) {
	return KMalloc::get().kmalloc_alloc(size);
}

void* operator new[](size_t size) {
	return KMalloc::get().kmalloc_alloc(size);
}

void operator delete(void*, size_t) {

}

void operator delete[](void*, size_t) {

}
