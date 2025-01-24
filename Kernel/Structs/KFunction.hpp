#pragma once
#include <Core/Assert/Panic.hpp>
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Move.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <string.h>

class __StaticFunctionStorage {
public:
	using PtrType = void*;
	using StoragePointer = char*;
	static constexpr const size_t max_storage_size = sizeof(PtrType);
public:
	constexpr __StaticFunctionStorage()
	    : m_storage_size(0)
	    , m_storage() {}

	//  Construct storage for a specific lambda type T.
	//  The lambda must fit within the size specified by `max_storage_size`,
	//  as the storage is inline and not heap-allocated.
	template<typename T>
	constexpr explicit __StaticFunctionStorage(T storage_type)
	    : m_storage_size(sizeof(T))
	    , m_storage() {
		static_assert(can_fit_storage(sizeof(storage_type)), "Lambda storage does not fit within a single pointer");
		m_storage_size = sizeof(T);
	}

	//  Explicitly call the copy constructor from the move constructor.
	//  There is no meaningful "move" we can perform, as all storage is internal
	//  to the object.
	constexpr __StaticFunctionStorage(__StaticFunctionStorage&& storage) noexcept
	    : __StaticFunctionStorage(static_cast<__StaticFunctionStorage const&>(storage)) {}

	//  Copy storage from a different storage object.
	constexpr __StaticFunctionStorage(__StaticFunctionStorage const& storage)
	    : m_storage_size(storage.m_storage_size)
	    , m_storage() {
		__builtin_memcpy(m_storage, storage.m_storage, storage.m_storage_size);
	}

	//  Default destructor, as there's nothing to do (storage is static).
	constexpr ~__StaticFunctionStorage() = default;

	//  Move assignment for the storage object, see move constructor for rationale.
	constexpr __StaticFunctionStorage& operator=(__StaticFunctionStorage&& storage) {
		*this = static_cast<__StaticFunctionStorage const&>(storage);
		return *this;
	}

	//  Copy storage from a different storage object.
	constexpr __StaticFunctionStorage& operator=(__StaticFunctionStorage const& storage) {
		m_storage_size = storage.m_storage_size;
		__builtin_memcpy(m_storage, storage.m_storage, storage.m_storage_size);
		return *this;
	}

	constexpr StoragePointer get() { return m_storage; }

	[[nodiscard]] constexpr size_t size() const { return m_storage_size; }
private:
	static constexpr bool can_fit_storage(size_t size) { return size <= max_storage_size; }

	size_t m_storage_size;
	char m_storage[max_storage_size];
};

template<typename Functor>
concept LambdaCapturesOccupyASinglePointer = sizeof(Functor) <= __StaticFunctionStorage::max_storage_size;

template<typename T>
class KFunction;

///  Object for storing generic functors with a static storage duration.
///
///  This is a replacement for std::function that does not use heap for
///  allocation of the functor storage, which makes it suitable for use
///  within the kernel. Functor state is stored directly in the KFunction
///  object itself - the max size of the stored state is defined by the
///  value of `__StaticFunctionStorage::max_storage_size`.
template<typename Ret, typename... Args>
class KFunction<Ret(Args...)> {
	using InvokeFunc = Ret (*)(char*, Args&&...);
	using ConstructFunc = void (*)(char*, char*);
	using DestroyFunc = void (*)(char*);

	ConstructFunc m_construct_func;
	InvokeFunc m_invoke_func;
	DestroyFunc m_destroy_func;

	__StaticFunctionStorage m_storage;

	template<typename Functor>
	static constexpr Ret invoke_functor(Functor* fn, Args&&... args) {
		return (*fn)(gen::forward<Args>(args)...);
	}

	template<typename Functor>
	static constexpr void construct_functor(Functor* destination, Functor* source) {
		new(destination) Functor(*source);
	}

	template<typename Functor>
	static constexpr void destroy_functor(Functor* fn) {
		fn->~Functor();
	}

	constexpr void destruct() {
		if(m_destroy_func != nullptr) {
			m_destroy_func(m_storage.get());
		}
	}
public:
	//  Default constructor for a null functor object.
	//  Calling the functor in null state is not allowed and will cause a panic.
	constexpr KFunction()
	    : m_construct_func(nullptr)
	    , m_invoke_func(nullptr)
	    , m_destroy_func(nullptr)
	    , m_storage() {}

	//  Construct a functor from a callable.
	template<typename Functor>
	requires LambdaCapturesOccupyASinglePointer<Functor>
	constexpr KFunction(Functor f)
	    : m_construct_func(reinterpret_cast<ConstructFunc>(construct_functor<Functor>))
	    , m_invoke_func(reinterpret_cast<InvokeFunc>(invoke_functor<Functor>))
	    , m_destroy_func(reinterpret_cast<DestroyFunc>(destroy_functor<Functor>))
	    , m_storage(f) {
		m_construct_func(m_storage.get(), reinterpret_cast<char*>(&f));
	}

	//  Copy construct can be defaulted, as we don't need any special handling.
	//  The entire idea behind KFunction objects is all resources tracked are part
	//  of the object itself. There is no special handling required, as no heap
	//  allocations are done.
	constexpr KFunction(KFunction const&) = default;

	//  Move constructor can be defaulted, as we don't need any special handling.
	//  Moves don't make much sense, as we end up copying everything anyway.
	constexpr KFunction(KFunction&& f) = default;

	//  Functor destructor.
	//	This calls the destructor associated with the stored lambda.
	constexpr ~KFunction() { destruct(); }

	//  Move assignment can be defaulted.
	constexpr KFunction& operator=(KFunction&& f) = default;

	//  Copy assignment can be defaulted.
	constexpr KFunction& operator=(KFunction const&) = default;

	//  Check if the functor is in a valid state (not null).
	constexpr operator bool() { return !is_null(); }

	[[nodiscard]] constexpr bool is_null() const { return !m_invoke_func; }

	//  Invoke the stored functor and return the result from the call. Attempting
	//  to invoke the functor when the object is in null state triggers a panic.
	constexpr Ret operator()(Args&&... args) {
		if(is_null()) {
			core::panic("Call to KFunction object in null state!");
		}
		return m_invoke_func(m_storage.get(), gen::forward<Args>(args)...);
	}

	//  Functional wrapper around operator()
	constexpr Ret invoke(Args&&... args) { return this->operator()(gen::forward<Args>(args)...); }
};