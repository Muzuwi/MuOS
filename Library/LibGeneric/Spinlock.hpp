#pragma once
#include <stdint.h>

namespace gen {

	class Spinlock {
		volatile uint64_t m_lock {0};
	public:
		void lock() {
			while(__sync_lock_test_and_set(&m_lock, 1) != 0)
				;
		}

		bool try_lock() {
			return !__sync_lock_test_and_set(&m_lock, 1);
		}

		void unlock() {
			__sync_lock_release(&m_lock);
		}
	};


}