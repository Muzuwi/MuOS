#include <Debug/klogf.hpp>
#include <Debug/kpanic.hpp>
#include <LibGeneric/Allocator.hpp>
#include <Memory/Allocators/ChunkAllocator.hpp>
#include <string.h>

ChunkAllocator::ChunkAllocator(void* virtual_start, size_t size)
    : m_chunk_list(new(virtual_start) Chunk(size - sizeof(Chunk)))
    , m_region_size(size)
    , m_start(virtual_start) {}

void* ChunkAllocator::allocate(size_t size) {
	auto chunk = find_free_chunk(size);
	if(chunk) {
		mark_chunk_allocated(*chunk, size);
		//		klogf_static("Alloc {} -> {}\n", size, chunk->alloc_ptr());
		return chunk->alloc_ptr();
	}
	return nullptr;
}

void ChunkAllocator::free(void* ptr) {
	if(!ptr) {
		return;
	}

	auto* chunk = find_chunk_with_address(ptr);
	if(!chunk) {
		return;
	}

	//  Tried freeing an object from an offset pointer
	if(ptr != chunk->alloc_ptr()) {
		kerrorf_static("Partial free of pointer {} detected!\n", ptr);
		dump_allocator();
		kpanic();
	}

	//  Tried double-freeing
	if(chunk->m_state == ChunkState::Free) {
		kerrorf_static("Double free of pointer {} detected!\n", ptr);
		dump_allocator();
		kpanic();
	}

	chunk->m_state = ChunkState::Free;
}

Chunk* ChunkAllocator::find_free_chunk(size_t size) {
	auto current = m_chunk_list;
	while(current) {
		if(current->m_state == ChunkState::Free && current->m_size >= size) {
			return current;
		}
		current = current->m_next;
	}

	return nullptr;
}

bool ChunkAllocator::mark_chunk_allocated(Chunk& chunk, size_t size) {
	constexpr const size_t minimum_chunk_data_size = 8;
	constexpr const uint8 sanitize_byte = 0xBA;

	//  Region is too small
	if(chunk.m_size < size) {
		return false;
	}

	const auto size_after_alloc = chunk.m_size - size;
	//  If the chunk is too small to split (doesn't have enough space to contain the chunk structure
	//  and the amount of left free space would be meaningless), don't split into new chunks and live with the
	//  overcommit
	if(size_after_alloc < sizeof(Chunk) + minimum_chunk_data_size) {
		//  Mark it as allocated - we're done
		chunk.m_state = ChunkState::Allocated;
		memset(chunk.alloc_ptr(), sanitize_byte, size);
		return true;
	}

	//  We can split the chunk into a new chunk with a sensible size
	chunk.m_size = size;
	chunk.m_state = ChunkState::Allocated;
	memset(chunk.alloc_ptr(), sanitize_byte, size);
	auto* new_chunk_ptr = reinterpret_cast<uint8*>(chunk.alloc_ptr()) + size;
	const auto new_chunk_size = size_after_alloc - sizeof(Chunk);
	auto* new_chunk = new(new_chunk_ptr) Chunk(new_chunk_size);
	memset(new_chunk->alloc_ptr(), sanitize_byte, new_chunk_size);

	auto* previous_next = chunk.m_next;
	chunk.m_next = new_chunk;
	new_chunk->m_next = previous_next;

	return true;
}

Chunk* ChunkAllocator::find_chunk_with_address(void* addr) {
	auto current = m_chunk_list;
	while(current) {
		if(addr >= current->alloc_ptr() && addr < current->alloc_end_ptr()) {
			return current;
		}
		current = current->m_next;
	}
	return nullptr;
}

void ChunkAllocator::dump_allocator() {
	auto state_str = [](ChunkState state) -> char const* {
		switch(state) {
			case ChunkState::Free: return "Free";
			case ChunkState::Allocated: return "Allocated";
			default: return "Invalid";
		}
	};

	auto* current = m_chunk_list;
	while(current) {
		kerrorf_static("Chunk({}): {} <-> {}, size{{{}}}, state{{{}}}, next{{{}}}\n", Format::ptr(current),
		               current->alloc_ptr(), current->alloc_end_ptr(), current->m_size, state_str(current->m_state),
		               Format::ptr(current->m_next));
		current = current->m_next;
	}
}
