#include <SystemTypes.hpp>
#include <Structs/KAtomic.hpp>

template<typename T>
T KAtomic<T>::load(MemoryOrdering ordering) {
	T value;
	__atomic_load(&m_value, &value, static_cast<int>(ordering));
	return value;
}

template<typename T>
void KAtomic<T>::store(T value, MemoryOrdering ordering) {
	__atomic_store(&m_value, &value, static_cast<int>(ordering));
}

template<typename T>
T KAtomic<T>::exchange(T value, MemoryOrdering ordering) {
	T ret;
	__atomic_exchange(&m_value, &value, &ret, static_cast<int>(ordering));
	return ret;
}

template uint64 KAtomic<uint64>::load(MemoryOrdering);
template uint32 KAtomic<uint32>::load(MemoryOrdering);
template uint16 KAtomic<uint16>::load(MemoryOrdering);
template uint8  KAtomic<uint8>::load(MemoryOrdering);
template int64 KAtomic<int64>::load(MemoryOrdering);
template int32 KAtomic<int32>::load(MemoryOrdering);
template int16 KAtomic<int16>::load(MemoryOrdering);
template int8  KAtomic<int8>::load(MemoryOrdering);
template void KAtomic<uint64>::store(uint64, MemoryOrdering);
template void KAtomic<uint32>::store(uint32, MemoryOrdering);
template void KAtomic<uint16>::store(uint16, MemoryOrdering);
template void KAtomic<uint8>::store(uint8, MemoryOrdering);
template void KAtomic<int64>::store(int64, MemoryOrdering);
template void KAtomic<int32>::store(int32, MemoryOrdering);
template void KAtomic<int16>::store(int16, MemoryOrdering);
template void KAtomic<int8>::store(int8, MemoryOrdering);
template uint64 KAtomic<uint64>::exchange(uint64, MemoryOrdering);
template uint32 KAtomic<uint32>::exchange(uint32, MemoryOrdering);
template uint16 KAtomic<uint16>::exchange(uint16, MemoryOrdering);
template uint8 KAtomic<uint8>::exchange(uint8, MemoryOrdering);
template int64 KAtomic<int64>::exchange(int64, MemoryOrdering);
template int32 KAtomic<int32>::exchange(int32, MemoryOrdering);
template int16 KAtomic<int16>::exchange(int16, MemoryOrdering);
template int8 KAtomic<int8>::exchange(int8, MemoryOrdering);
