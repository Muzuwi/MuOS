#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Memory/VirtualMemManager.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <string.h>

//#define KMALLOC_DEBUG
//#define KMALLOC_DEBUG_NOISY

#define KMALLOC_SANITIZER
#define KMALLOC_SANITIZER_BYTE 0x99

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

	memset((void*)m_mem_allocations, 0, KMALLOC_ARR_COUNT*sizeof(chunk_t));

	m_kmalloc_mem_range = VirtualMemManager::get().bootstrap(KMALLOC_POOL_SIZE);
}

/*
 *	Debug function that logs the current allocation statistics to the kernel log
 */
void KMalloc::logAllocationStats() {
	kdebugf("[KMalloc] Current allocations: %i bytes\n", m_current_allocations);
	kdebugf("[KMalloc] Total allocations: %i bytes\n", m_total_allocations);
	kdebugf("[KMalloc] Total frees: %i bytes\n", m_total_frees);
	kdebugf("[KMalloc] Detailed allocation breakdown:\n");

	size_t chunk = 0;
	bool any = false;
	while(chunk < KMALLOC_ARR_COUNT*bits(chunk_t)) {
		if(bitmask(chunk % bits(chunk_t)) & m_mem_allocations[chunk / bits(chunk_t)]) {
			any = true;
			uint32_t mem_offset = chunk*KMALLOC_CHUNK;
			uint32_t* memory_begin = (uint32_t*)(m_kmalloc_mem_range.m_start + mem_offset);
			uint32_t* alloc_begin  = (uint32_t*)((uint32_t)memory_begin + sizeof(allocation_t));
			allocation_t alloc_size = *((allocation_t*)memory_begin);

			kdebugf("	- %x: %i bytes, chunk no. %i, alloc structure at %x\n",
			        (uint32_t)alloc_begin,
			        (uint32_t)alloc_size,
			        (uint32_t)chunk,
			        (uint32_t)memory_begin
			        );
			chunk += alloc_size / KMALLOC_CHUNK;
		}
		chunk++;
	}

	if(!any) {
		kdebugf("	- No memory currently allocated\n");
	}
}

/*
	This is just a helper to easily mark allocations that span 
	multiple rray entries as used
*/
void KMalloc::mark_range(size_t start_chunk, size_t end_chunk, bool clear=false) {

#ifdef KMALLOC_DEBUG_NOISY
	kdebugf("[KMalloc] marking chunks %i:%i as %s\n", start_chunk, end_chunk, clear ? "unalloc'd" : "alloc'd");
#endif

	for(size_t chunk = start_chunk; chunk <= end_chunk; chunk++) {
		size_t index = chunk / bits(chunk_t);

		if(clear) {
			if(!(m_mem_allocations[index] & (1 << (chunk % bits(chunk_t))))) {
				kerrorf("Tried clearing allocation bit when allocation is already clear (%i-%i)\n", start_chunk, end_chunk);
			}
			m_mem_allocations[index] &= ~(1 << (chunk % bits(chunk_t)));
		} else {
			if(m_mem_allocations[index] & (1 << (chunk % bits(chunk_t)))) {
				kerrorf("Tried setting allocation bit when allocation is already present (%i-%i)\n", start_chunk, end_chunk);
			}
			m_mem_allocations[index] |= (1 << (chunk % bits(chunk_t)));
		}
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
	size_t start = 0;

	for(size_t i = 0; i < KMALLOC_ARR_COUNT; i++){
		if(m_mem_allocations[i] == 0xffffffff) {
			available = 0;
			start = (i+1)*bits(chunk_t);
			continue;
		}

		for(size_t current = 0; current < bits(chunk_t); current++){
			if(!((1 << current) & m_mem_allocations[i])) {
				available++; 
			}else {
				available = 0;
				start = i*bits(chunk_t) + current+1;
				continue;
			}

			if(available >= chunks){
#ifdef KMALLOC_DEBUG_NOISY
				kdebugf("[KMalloc] available %i\n", available);
#endif
				mark_range(start, start+chunks-1);

				uint32_t mem_offset = start*KMALLOC_CHUNK;
				uint32_t* memory_begin = (uint32_t*)(m_kmalloc_mem_range.m_start + mem_offset);

#ifdef KMALLOC_DEBUG_NOISY
				kdebugf("[KMalloc] found at %x, chunks %i-%i, offset %x\n", memory_begin, (int)start, (int)(start+chunks-1), (uint32_t)mem_offset);
#endif

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
#endif
#ifdef KMALLOC_DEBUG_NOISY
	kdebugf("[KMalloc] allocation address: %x\n", address);
#endif

	allocation_t alloc = *((allocation_t*)address);

#ifdef KMALLOC_DEBUG_NOISY
	kdebugf("[KMalloc] allocation size: %i (actual: %i) [bytes]\n", alloc, (alloc % sizeof(allocation_t)) ? alloc + 1 : alloc);
#endif

	size_t start_chunk = (address - m_kmalloc_mem_range.m_start) / KMALLOC_CHUNK;
	size_t end_chunk = (start_chunk + (((alloc % sizeof(allocation_t)) ? alloc + 1 : alloc) / KMALLOC_CHUNK));

#ifdef KMALLOC_DEBUG_NOISY
	kdebugf("[KMalloc] start chunk: %i, end chunk: %i\n", start_chunk, end_chunk);
#endif

	mark_range(start_chunk, end_chunk, true);

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
