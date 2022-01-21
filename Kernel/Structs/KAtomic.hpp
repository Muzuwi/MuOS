#pragma once

enum class MemoryOrdering {
	Relaxed = __ATOMIC_RELAXED,
	Consume = __ATOMIC_CONSUME,
	Acquire = __ATOMIC_ACQUIRE,
	Release = __ATOMIC_RELEASE,
	AcqRel = __ATOMIC_ACQ_REL,
	SeqCst = __ATOMIC_SEQ_CST,
};

//  FIXME: This should only be used for primitive types, print fancy template errors otherwise instead of linker errors
template<typename T>
class KAtomic {
	T m_value;
public:
	KAtomic() = default;

	explicit KAtomic(T value) noexcept
			: m_value(value) {}

	~KAtomic() = default;

	/*
	 *  Atomically loads and returns the stored value
	 */
	[[nodiscard]] T load(MemoryOrdering);

	/*
	 *  Atomically stores the input value
	 */
	void store(T, MemoryOrdering);

	/*
	 *  Atomically stores the input value and returns the old contained value
	 */
	[[nodiscard]] T exchange(T value, MemoryOrdering);
};

