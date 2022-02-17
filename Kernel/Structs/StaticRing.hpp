#pragma once

#include <stddef.h>
#include <Structs/KAtomic.hpp>
#include <Structs/KOptional.hpp>

/*
 *  Implements a single-producer, single-consumer lock-free ring buffer
 */
template<class T, size_t count>
class StaticRing {
	T m_elements[count];
	KAtomic<size_t> m_read_pointer;
	KAtomic<size_t> m_write_pointer;
public:
	StaticRing() noexcept
	    : m_read_pointer()
	    , m_write_pointer() {
		m_read_pointer.store(0, MemoryOrdering::Relaxed);
		m_write_pointer.store(0, MemoryOrdering::Relaxed);
	}

	KOptional<T> try_pop() {
		const auto current = m_read_pointer.load(MemoryOrdering::SeqCst);
		if(current == m_write_pointer.load(MemoryOrdering::SeqCst)) {
			return {};
		}

		const auto value = m_elements[current];
		m_read_pointer.store((current + 1) % count, MemoryOrdering::SeqCst);
		return { value };
	}

	bool try_push(T value) {
		const auto current_tail = m_write_pointer.load(MemoryOrdering::SeqCst);
		const auto next_tail = (current_tail + 1) % count;
		if(next_tail == m_read_pointer.load(MemoryOrdering::SeqCst)) {
			return false;
		}

		m_elements[current_tail] = value;
		m_write_pointer.store(next_tail, MemoryOrdering::SeqCst);
		return true;
	}

	bool empty() { return m_read_pointer.load(MemoryOrdering::SeqCst) == m_write_pointer.load(MemoryOrdering::SeqCst); }
};
