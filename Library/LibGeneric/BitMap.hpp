#pragma once

#ifdef __is_kernel_build__
#	include <Core/Assert/Assert.hpp>
#endif

#include <stddef.h>
#include <stdint.h>

#define bits(a)    (sizeof(a) * 8)
#define bitmask(a) (size_t(1) << (a))

namespace gen {

	/*
	 *  Bitmap implementation, uses a uint32 to speed up searching in larger bitmaps,
	 *  at the cost of being wasteful when using a small element count
	 */
	class BitMap {
		static const size_t bits_per_entry = bits(uint32_t);

		uint32_t* m_bitmap;
		size_t m_count;
	public:
		/*
		 *  Constructs a bitmap with space for the specified amount of entries
		 */
		BitMap(size_t entries) {
			ENSURE(entries != 0);

			//  Align to size of a uint32
			m_count = (entries & 31u) ? (entries & ~31u) + bits_per_entry : (entries);
			m_bitmap = new uint32_t[m_count / bits_per_entry];

			this->clear();
		}

		/*
		 *  Copy-constructed bitmap
		 */
		BitMap(const BitMap& a) {
			m_count = a.m_count;
			m_bitmap = new uint32_t[m_count / bits_per_entry];

			for(unsigned i = 0; i < m_count / bits_per_entry; ++i) {
				m_bitmap[i] = a.m_bitmap[i];
			}
		}

		/*
		 *  Move-constructed bitmap
		 */
		BitMap(BitMap&& a) noexcept {
			m_count = a.m_count;
			m_bitmap = a.m_bitmap;

			a.m_bitmap = nullptr;
			a.m_count = 0;
		}

		~BitMap() {
			delete[] m_bitmap;
			m_bitmap = nullptr;
			m_count = 0;
		}

		/*
		 *  Returns the element count the bitmap was constructed with.
		 *  This is different from the size of the backing array, as it can be larger
		 *  than the amount of requested elements.
		 */
		size_t count() const { return m_count; }

		/*
		 *  Returns the value of the bit at index 'n'
		 */
		bool at(size_t n) const {
			ENSURE(n < m_count);

			size_t index = n / bits_per_entry;
			size_t bit = n % bits_per_entry;
			return m_bitmap[index] & bitmask(bit);
		}

		/*
		 *  Returns the value of the bit at index 'n'
		 */
		bool operator[](size_t n) const { return this->at(n); }

		/*
		 *  Sets a bit at index 'n' to the specified value, 1 by default
		 */
		void set(size_t n, bool v = true) {
			ENSURE(n < m_count);

			size_t index = n / bits_per_entry;
			size_t bit = n % bits_per_entry;
			if(v)
				m_bitmap[index] |= bitmask(bit);
			else
				m_bitmap[index] &= ~bitmask(bit);
		}

		/*
		 *  Resets all bits of the bitmap to 0
		 */
		void clear() {
			for(size_t i = 0; i < m_count / bits_per_entry; ++i)
				m_bitmap[i] = 0;
		}

		/*
		 *  Finds a sequence of 'n' consecutive cleared bits in the bitmap
		 *  Returns the starting index if such a sequence is found, otherwise returns count()
		 */
		size_t find_seq_clear(size_t n) const {
			size_t i = 0;
			size_t startIndex = m_count;
			while(i < m_count) {
				if(m_bitmap[i / bits_per_entry] == 0xffffffff) {
					i = (i + bits_per_entry) & ~31u;
					startIndex = m_count;
					continue;
				}

				bool val = this->at(i);

				if(val) {
					startIndex = m_count;
				} else {
					//  Found a potential start index
					if(startIndex == m_count)
						startIndex = i;

					if(startIndex != m_count && i - startIndex + 1 == n)
						break;
				}

				//  If ran out of entries while searching for sequence, reset start index
				if(i == m_count - 1 && startIndex != m_count)
					startIndex = m_count;

				++i;
			}

			return startIndex;
		}
	};

}