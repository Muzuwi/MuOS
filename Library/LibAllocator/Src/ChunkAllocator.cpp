#include <LibAllocator/Arena.hpp>
#include <LibAllocator/ChunkAllocator.hpp>
#include <string.h>
#ifndef __is_kernel_build__
#	include <new>
#else
#	include <LibGeneric/Allocator.hpp>
#	include <Core/Log/Logger.hpp>
#	include <Core/Assert/Assert.hpp>

CREATE_LOGGER("liballoc", core::log::LogLevel::Debug);

#endif

liballoc::ChunkAllocator::ChunkAllocator(liballoc::Arena arena)
    : m_chunk_list(new(arena.base) Chunk(arena.length - sizeof(Chunk))) {}

void* liballoc::ChunkAllocator::allocate(size_t size) {
	auto chunk = find_free_chunk(size);
	if(chunk) {
		mark_chunk_allocated(*chunk, size);
		return chunk->alloc_ptr();
	}
	return nullptr;
}

void liballoc::ChunkAllocator::free(void* ptr) {
	if(!ptr) {
		return;
	}

	auto* chunk = find_chunk_with_address(ptr);
	if(!chunk) {
		return;
	}

	//  Tried freeing an object from an offset pointer
	if(ptr != chunk->alloc_ptr()) {
#ifdef __is_kernel_build__
		log.fatal("Partial free of pointer {} detected!", ptr);
		ENSURE_NOT_REACHED();
#endif
		return;
	}

	//  Tried double-freeing
	if(chunk->state == ChunkState::Free) {
#ifdef __is_kernel_build__
		log.fatal("Double free of pointer {} detected!", ptr);
		ENSURE_NOT_REACHED();
#endif
		return;
	}

	chunk->state = ChunkState::Free;
}

liballoc::Chunk* liballoc::ChunkAllocator::find_free_chunk(size_t size) {
	auto current = m_chunk_list;
	while(current) {
		if(current->state == ChunkState::Free && current->size >= size) {
			return current;
		}
		current = current->next;
	}

	return nullptr;
}

bool liballoc::ChunkAllocator::mark_chunk_allocated(Chunk& chunk, size_t size) {
	constexpr const size_t minimum_chunk_data_size = 8;
	constexpr const uint8_t sanitize_byte = 0xBA;

	//  Region is too small
	if(chunk.size < size) {
		return false;
	}

	const auto size_after_alloc = chunk.size - size;
	//  If the chunk is too small to split (doesn't have enough space to contain the chunk structure
	//  and the amount of left free space would be meaningless), don't split into new chunks and live with the
	//  overcommit
	if(size_after_alloc < sizeof(Chunk) + minimum_chunk_data_size) {
		//  Mark it as allocated - we're done
		chunk.state = ChunkState::Allocated;
		memset(chunk.alloc_ptr(), sanitize_byte, size);
		return true;
	}

	//  We can split the chunk into a new chunk with a sensible size
	chunk.size = size;
	chunk.state = ChunkState::Allocated;
	memset(chunk.alloc_ptr(), sanitize_byte, size);
	auto* new_chunk_ptr = reinterpret_cast<uint8_t*>(chunk.alloc_ptr()) + size;
	const auto new_chunk_size = size_after_alloc - sizeof(Chunk);
	auto* new_chunk = new(new_chunk_ptr) Chunk(new_chunk_size);

	auto* previous_next = chunk.next;
	chunk.next = new_chunk;
	new_chunk->next = previous_next;

	return true;
}

liballoc::Chunk* liballoc::ChunkAllocator::find_chunk_with_address(void* addr) {
	auto current = m_chunk_list;
	while(current) {
		if(addr >= current->alloc_ptr() && addr < current->alloc_end_ptr()) {
			return current;
		}
		current = current->next;
	}
	return nullptr;
}
