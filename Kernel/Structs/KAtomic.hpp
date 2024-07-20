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
concept SupportsAtomicLoad = requires(T* ptr, T* ret, MemoryOrdering order) {
	{ __atomic_load(ptr, ret, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicLoadN = requires(T* ptr, MemoryOrdering order) {
	{ __atomic_load_n(ptr, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicStore = requires(T* ptr, T* val, MemoryOrdering order) {
	{ __atomic_store(ptr, val, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicStoreN = requires(T* ptr, T val, MemoryOrdering order) {
	{ __atomic_store_n(ptr, val, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicExchange = requires(T* ptr, T* val, T* ret, MemoryOrdering order) {
	{ __atomic_exchange(ptr, val, ret, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicExchangeN = requires(T* ptr, T val, MemoryOrdering order) {
	{ __atomic_exchange_n(ptr, val, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicFetchAdd = requires(T* ptr, T val, MemoryOrdering order) {
	{ __atomic_fetch_add(ptr, val, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicFetchSub = requires(T* ptr, T val, MemoryOrdering order) {
	{ __atomic_fetch_sub(ptr, val, static_cast<int>(order)) };
};

template<typename T>
concept SupportsAtomicCompareExchange = requires(T* ptr, T* expected, T desired) {
	{
		__atomic_compare_exchange_n(ptr, expected, desired, false, static_cast<int>(MemoryOrdering::Release),
		                            static_cast<int>(MemoryOrdering::Relaxed))
	};
};

template<typename T>
class KAtomic {
public:
	constexpr KAtomic() = default;

	constexpr explicit KAtomic(T value) noexcept
	    : m_value(value) {}

	constexpr ~KAtomic() = default;

	/*
	 *  Atomically loads and returns the stored value
	 */
	[[nodiscard]] T load(MemoryOrdering ordering)
	requires SupportsAtomicLoad<T>
	{
		T value;
		__atomic_load(&m_value, &value, static_cast<int>(ordering));
		return value;
	}

	/*
	 *  Atomically stores the input value
	 */
	void store(T value, MemoryOrdering ordering)
	requires SupportsAtomicStore<T>
	{
		//  clang-format: off
		__atomic_store(&m_value, &value, static_cast<int>(ordering));
		//  clang-format: on
	}

	/*
	 *  Atomically stores the input value and returns the old contained value
	 */
	[[nodiscard]] T exchange(T value, MemoryOrdering ordering)
	requires SupportsAtomicExchange<T>
	{
		T ret;
		__atomic_exchange(&m_value, &value, &ret, static_cast<int>(ordering));
		return ret;
	}

	/*
	 *	Performs a strong CAS operation on the value.
	 *
	 * 	This is a wrapper around __atomic_compare_exchange_n. If the current
	 * 	value does not equal expected, expected is updated with the current
	 * 	value. Otherwise, the current value is updated to desired.
	 */
	bool compare_exchange_strong(T& expected, T desired, MemoryOrdering success_order, MemoryOrdering fail_order)
	requires SupportsAtomicCompareExchange<T>
	{
		return __atomic_compare_exchange_n(&m_value, &expected, desired, false, static_cast<int>(success_order),
		                                   static_cast<int>(fail_order));
	}

	/*
	 *	Performs a weak CAS operation on the value.
	 *
	 * 	This is a wrapper around __atomic_compare_exchange_n. If the current
	 * 	value does not equal expected, expected is updated with the current
	 * 	value. Otherwise, the current value is updated to desired.
	 */
	bool compare_exchange_weak(T& expected, T desired, MemoryOrdering success_order, MemoryOrdering fail_order)
	requires SupportsAtomicCompareExchange<T>
	{
		return __atomic_compare_exchange_n(&m_value, &expected, desired, true, static_cast<int>(success_order),
		                                   static_cast<int>(fail_order));
	}

	/*
	 *	Atomically adds the given value and returns the old contained value.
	 */
	[[nodiscard]] T fetch_add(T value, MemoryOrdering ordering)
	requires SupportsAtomicFetchAdd<T>
	{
		return __atomic_fetch_add(&m_value, value, static_cast<int>(ordering));
	}

	/*
	 *	Atomically subtracts the given value and returns the old contained value.
	 */
	[[nodiscard]] T fetch_sub(T value, MemoryOrdering ordering)
	requires SupportsAtomicFetchSub<T>
	{
		return __atomic_fetch_sub(&m_value, value, static_cast<int>(ordering));
	}
private:
	T m_value;
};
