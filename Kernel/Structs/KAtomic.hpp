#pragma once

enum class MemoryOrdering {
	Relaxed = __ATOMIC_RELAXED,
	Consume = __ATOMIC_CONSUME,
	Acquire = __ATOMIC_ACQUIRE,
	Release = __ATOMIC_RELEASE,
	AcqRel = __ATOMIC_ACQ_REL,
	SeqCst = __ATOMIC_SEQ_CST,
};

template<typename T>
concept SupportsKernelAtomics = requires(T oldval) {
	{ __atomic_load_n(&oldval, static_cast<int>(MemoryOrdering::SeqCst)) };
	{ __atomic_store_n(&oldval, T {}, static_cast<int>(MemoryOrdering::SeqCst)) };
	{ __atomic_exchange_n(&oldval, T {}, static_cast<int>(MemoryOrdering::SeqCst)) };
};

template<SupportsKernelAtomics T>
class KAtomic {
public:
	constexpr KAtomic() = default;

	constexpr explicit KAtomic(T value) noexcept
	    : m_value(value) {}

	constexpr ~KAtomic() = default;

	/*
	 *  Atomically loads and returns the stored value
	 */
	[[nodiscard]] T load(MemoryOrdering ordering) {
		T value;
		__atomic_load(&m_value, &value, static_cast<int>(ordering));
		return value;
	}

	/*
	 *  Atomically stores the input value
	 */
	void store(T value, MemoryOrdering ordering) {
		//  clang-format: off
		__atomic_store(&m_value, &value, static_cast<int>(ordering));
		//  clang-format: on
	}

	/*
	 *  Atomically stores the input value and returns the old contained value
	 */
	[[nodiscard]] T exchange(T value, MemoryOrdering ordering) {
		T ret;
		__atomic_exchange(&m_value, &value, &ret, static_cast<int>(ordering));
		return ret;
	}
private:
	T m_value;
};
