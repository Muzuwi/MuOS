#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Memory/VirtualMemManager.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <string.h>

//#define KMALLOC_DEBUG
#define KMALLOC_SANITIZER
#define KMALLOC_SANITIZER_BYTE 0x0

chunk_t m_mem_allocations[KMALLOC_ARR_COUNT] {

};

KMalloc::KMalloc() {
	m_total_allocations = 0;
	m_current_allocations = 0;
	m_total_frees = 0;
}


KMalloc& KMalloc::get() {
	static KMalloc km;
	return km;
}

void KMalloc::init() {
	m_kmalloc_mem_range = mem_range_t(0, 0);

	memset((void*)m_mem_allocations, 0, KMALLOC_ARR_COUNT);

	m_kmalloc_mem_range = VirtualMemManager::get().bootstrap(KMALLOC_POOL_SIZE);
}


/*
	This is just a helper to easily mark allocations that span 
	multiple rray entries as used
*/
void KMalloc::mark_range(size_t start_index, size_t end_index, size_t start_chunk, size_t end_chunk, bool clear=false) {
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

		for(size_t chunk = start; chunk < upper_bound; chunk++) {
			if(clear) {
				m_mem_allocations[i] &= ~(1 << chunk);
			} else {
				m_mem_allocations[i] |= (1 << chunk);
			}
		}

		if(start_index == end_index) return;
	}

}

/*
 *  Allocates a memory region
 */
void* KMalloc::kmalloc_alloc(size_t size) {
	//  We're using a structure at the beginning of the 
	//  allocated memory to make freeing memory easier
	size_t proper_size = size + sizeof(allocation_t);
	size_t chunks = proper_size / KMALLOC_CHUNK;
	if(proper_size % KMALLOC_CHUNK) chunks++;

	size_t available = 0;
	size_t start = 0, start_index = 0;

	for(size_t i = 0; i < KMALLOC_ARR_COUNT; i++){
		if(m_mem_allocations[i] == 0xffffffff) {
			available = 0;
			start = 0;
			start_index = i+1;
			continue;
		}

		for(size_t current = 0; current < sizeof(chunk_t)*8; current++){
			if(!((1 << current) & m_mem_allocations[i])) {
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
				uint32_t* memory_begin = (uint32_t*)(m_kmalloc_mem_range.m_start + mem_offset);
				// kdebugf("[KMalloc] found at %x, chunks %i-%i, offset %x\n", memory_begin, (int)start, (int)(start+chunks-1), (uint32_t)mem_offset);

				*(allocation_t*)memory_begin = (allocation_t)size;

				void* allocated_begin = (void*)((uint32_t)memory_begin+sizeof(allocation_t));

                #ifdef KMALLOC_DEBUG
				kdebugf("[KMalloc] allocated %i bytes to %x\n", size, (uint32_t)allocated_begin);
                #endif

				#ifdef KMALLOC_SANITIZER
				memset((void*)allocated_begin, KMALLOC_SANITIZER_BYTE, size);
				#endif

				m_total_allocations += proper_size;
				m_current_allocations += proper_size;

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

/*
 *  Frees a kmalloc-allocated memory region
 */
void KMalloc::kmalloc_free(void* pointer) {
	if(!is_kmalloc_memory(pointer)) return;

	uintptr_t address = (uintptr_t)pointer - sizeof(allocation_t);

#ifdef KMALLOC_DEBUG
	kdebugf("[KMalloc] freeing address %x\n", (uint32_t)pointer);
	kdebugf("[KMalloc] allocation address: %x\n", address);
#endif

	allocation_t alloc = *((allocation_t*)address);

#ifdef KMALLOC_DEBUG
	kdebugf("[KMalloc] allocation size: %x\n", alloc);
#endif

	size_t start_chunk = (address - m_kmalloc_mem_range.m_start) / KMALLOC_CHUNK;
	size_t end_chunk = (start_chunk + (alloc/KMALLOC_CHUNK));
	size_t start_index = start_chunk / (sizeof(chunk_t)*8);
	size_t end_index = end_chunk / (sizeof(chunk_t)*8);

#ifdef KMALLOC_DEBUG
	kdebugf("[KMalloc] start chunk: %x, end chunk: %x\n", start_chunk, end_chunk);
	kdebugf("[KMalloc] start index: %x, end index: %x\n", start_index, end_index);
#endif

	mark_range(start_index, end_index, start_chunk, end_chunk, true);

	m_total_frees += (alloc + sizeof(allocation_t));
	m_current_allocations -= (alloc + sizeof(allocation_t));
}

/*
 *  Checks whether a given pointer is kmalloc memory
 */
bool KMalloc::is_kmalloc_memory(void* pointer) {
	return (uintptr_t)pointer >= m_kmalloc_mem_range.m_start &&
	        (uintptr_t)pointer < m_kmalloc_mem_range.m_end;
}

/*
 *  Returns the current amount of memory allocated by kmalloc
 */
uint64_t KMalloc::getCurrentAllocations() {
	return m_current_allocations;
}

/*
 *	Returns the total amount of allocations done by kmalloc
 */
uint64_t KMalloc::getTotalAllocations() {
	return m_total_allocations;
}

/*
 *	Returns the total amount of frees done by kmalloc
 */
uint64_t KMalloc::getTotalFrees() {
	return m_total_frees;
}


/*
 *  A variety of allocator functions
 */


void* operator new(size_t size) {
	return KMalloc::get().kmalloc_alloc(size);
}

void* operator new[](size_t size) {
	return KMalloc::get().kmalloc_alloc(size);
}

void operator delete(void* pointer, size_t) {
	KMalloc::get().kmalloc_free(pointer);
}

void operator delete(void* pointer) {
	KMalloc::get().kmalloc_free(pointer);
}

void operator delete[](void* pointer, size_t) {
	KMalloc::get().kmalloc_free(pointer);
}

void operator delete[](void* pointer) {
	KMalloc::get().kmalloc_free(pointer);
}
