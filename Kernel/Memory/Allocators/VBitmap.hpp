#pragma once

#include <Structs/KOptional.hpp>
#include <SystemTypes.hpp>

class VBitmap {
	void* m_base;
	size_t m_buffer_size;
	size_t m_entries;
	size_t m_used;

	static inline int64 divround(const int64 n, const int64 d) {
		return ((n < 0) ^ (d < 0)) ? ((n - d / 2) / d) : ((n + d / 2) / d);
	}

	inline bool bit_get(size_t idx) {
		const auto index = idx / 8;
		const auto offset = idx % 8;
		auto const& byte = *(reinterpret_cast<uint8*>(m_base) + index);

		return byte & (1u << offset);
	}

	inline void bit_set(size_t idx, bool value) {
		const auto index = idx / 8;
		const auto offset = idx % 8;
		auto& byte = *(reinterpret_cast<uint8*>(m_base) + index);

		byte &= ~(1u << offset);
		byte |= (value ? 1 : 0) << offset;
	}

	KOptional<size_t> find_one();

	KOptional<size_t> find_many(size_t);

	void mark_bits(size_t idx, size_t count, bool value);

	KOptional<size_t> allocate_impl(size_t count);

	void free_impl(size_t idx, size_t count);
public:
	VBitmap();

	VBitmap(void* base, size_t entries);

	size_t used() const { return m_used; }

	size_t entries() const { return m_entries; }

	size_t buffer_size() const { return m_buffer_size; }

	KOptional<size_t> allocate(size_t count = 1) { return allocate_impl(count); }

	void free(size_t idx, size_t alloc_size) { free_impl(idx, alloc_size); }
};