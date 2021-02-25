#include <string.h>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Symbols.hpp>
#include <LibGeneric/BitMap.hpp>
#include <LibGeneric/Mutex.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/StdRequired.hpp>

//#define KMALLOC_DEBUG
//#define KMALLOC_DEBUG_NOISY

#define KMALLOC_SANITIZER
#define KMALLOC_SANITIZER_BYTE 0x99

KMalloc::Chunk m_mem_allocations[KMalloc::array_count()] {};
static gen::Mutex s_kmalloc_lock {};

KMalloc::KMalloc() {
	m_total_allocations = 0;
	m_current_allocations = 0;
	m_total_frees = 0;
	this->init();
}


KMalloc& KMalloc::get() {
	static KMalloc km;
	return km;
}

void KMalloc::init() {
	memset((void*)m_mem_allocations, 0, array_count() * sizeof(Chunk));

	m_kmalloc_mem_range = mem_range_t((uint64_t)&_ukernel_kmalloc_start,
	                                  (uint64_t)&_ukernel_kmalloc_end);
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
		size_t index = chunk / bits(Chunk);

		if(clear) {
			if(!(m_mem_allocations[index] & (1 << (chunk % bits(Chunk))))) {
				kerrorf("Tried clearing allocation bit when allocation is already clear (%i-%i)\n", start_chunk, end_chunk);
				kpanic();
			}
			m_mem_allocations[index] &= ~(1 << (chunk % bits(Chunk)));
		} else {
			if(m_mem_allocations[index] & (1 << (chunk % bits(Chunk)))) {
				kerrorf("Tried setting allocation bit when allocation is already present (%i-%i)\n", start_chunk, end_chunk);
				kpanic();
			}
			m_mem_allocations[index] |= (1 << (chunk % bits(Chunk)));
		}
	}
}

/*
 *  Allocates a memory region
 */
void* KMalloc::kmalloc_alloc(size_t size, size_t align) {
	gen::LockGuard<gen::Mutex> lock {s_kmalloc_lock};

	//  We're using a structure at the beginning of the
	//  allocated memory to make freeing memory easier
	size_t proper_size = size + sizeof(allocation_t);
	size_t chunks = proper_size / chunk_size();
	if(proper_size % chunk_size()) chunks++;

	int start = -1;
	size_t available = 0;
	size_t chunk =0;
	while(chunk < array_count()*bits(Chunk)) {
		size_t arr_index = chunk / bits(Chunk);
		if(m_mem_allocations[arr_index] == 0xFFFFFFFF) {
			chunk += bits(Chunk);
			continue;
		}

		size_t mask = bitmask(chunk % bits(Chunk));

		bool isAligned = true;
		if(align != 1) {
			auto mem_offset = chunk*chunk_size();
			auto* memory_begin = (void*)(m_kmalloc_mem_range.m_start + mem_offset);
			auto allocated_begin = (uint64_t)memory_begin+sizeof(allocation_t);

			isAligned = (uint64_t)allocated_begin % align == 0;
		}

		if(!(mask & m_mem_allocations[arr_index])) {
			if(start == -1 && isAligned)
				start = chunk;
			available++;
		} else {
			available = 0;
			start = -1;
		}

		if(start != -1 && available >= chunks) {
#ifdef KMALLOC_DEBUG_NOISY
			kdebugf("[KMalloc] found %i available chunks\n", available);
			kdebugf("[KMalloc] starting: %i, ending: %i\n", start, start+available);
#endif
			mark_range(start, start+chunks-1);

			auto mem_offset = start*chunk_size();
			auto* memory_begin = (void*)(m_kmalloc_mem_range.m_start + mem_offset);

#ifdef KMALLOC_DEBUG_NOISY
			kdebugf("[KMalloc] found at %x, chunks %i-%i, offset %x\n", memory_begin, (int)start, (int)(start+chunks-1), (uint32_t)mem_offset);
#endif

			*(allocation_t*)memory_begin = (allocation_t)size;

			void* allocated_begin = (void*)((uint64_t)memory_begin+sizeof(allocation_t));

#ifdef KMALLOC_DEBUG
			kdebugf("[KMalloc] allocated %i bytes to %x%x\n", size, (uint64_t)allocated_begin>>32u, (uint64_t)allocated_begin&0xffffffffu);
			kdebugf("[KMalloc]  ... chunk start %x, chunk end %x\n", start, start+chunks-1);
#endif

#ifdef KMALLOC_SANITIZER
			memset((void*)allocated_begin, KMALLOC_SANITIZER_BYTE, size);
#endif

			m_total_allocations += chunks*chunk_size();
			m_current_allocations += chunks*chunk_size();

			return allocated_begin;
		}

		chunk++;
	}

	kerrorf("[KMalloc] cannot find %i free chunks for %i allocation (out of memory?)\n", chunks, size);
	kpanic();

}

/*
 *  Frees a kmalloc-allocated memory region
 */
void KMalloc::kmalloc_free(void* pointer) {
	if(!is_kmalloc_memory(pointer)) return;

	gen::LockGuard<gen::Mutex> lock {s_kmalloc_lock};

	uintptr_t address = (uintptr_t)pointer - sizeof(allocation_t);
#ifdef KMALLOC_DEBUG
	kdebugf("[KMalloc] freeing address %x%x\n", (uint64_t)pointer>>32u, (uint64_t)pointer&0xffffffffu);
#endif
#ifdef KMALLOC_DEBUG_NOISY
	kdebugf("[KMalloc] allocation address: %x\n", address);
#endif

	allocation_t alloc = *((allocation_t*)address);

#ifdef KMALLOC_DEBUG_NOISY
	kdebugf("[KMalloc] allocation size: %i (actual: %i) [bytes]\n", alloc, (alloc % sizeof(allocation_t)) ? alloc + 1 : alloc);
#endif

	size_t start_chunk = (address - m_kmalloc_mem_range.m_start) / chunk_size();
	size_t end_chunk = (start_chunk + (((alloc % sizeof(allocation_t)) ? alloc + 1 : alloc) / chunk_size()));

#ifdef KMALLOC_DEBUG_NOISY
	kdebugf("[KMalloc] start chunk: %i, end chunk: %i\n", start_chunk, end_chunk);
#endif

	mark_range(start_chunk, end_chunk, true);

	size_t size = (end_chunk - start_chunk + 1) * chunk_size();
	m_total_frees += size;
	m_current_allocations -= size;
}

/*
 *  Checks whether a given pointer is kmalloc memory
 */
bool KMalloc::is_kmalloc_memory(void* pointer) {
	return (uintptr_t)pointer >= m_kmalloc_mem_range.m_start &&
	        (uintptr_t)pointer < m_kmalloc_mem_range.m_end;
}

static inline size_t wrap_common_alignments(size_t size) {
	switch (size) {
		case 8: return 16;
		case 4: return 4;
		case 2: return 2;
		default: return 1;
	}
}