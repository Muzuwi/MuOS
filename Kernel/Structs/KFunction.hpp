#pragma once
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

	template<typename T>
	constexpr explicit __StaticFunctionStorage(T storage_type)
	    : m_storage_size(sizeof(T))
	    , m_storage() {
		static_assert(can_fit_storage(sizeof(storage_type)), "Lambda storage does not fit within a single pointer");
		m_storage_size = sizeof(T);
	}

	constexpr __StaticFunctionStorage(__StaticFunctionStorage&& storage) noexcept
	    : m_storage_size(storage.m_storage_size)
	    , m_storage() {
		m_storage_size = storage.m_storage_size;
		__builtin_memcpy(m_storage, storage.m_storage, storage.m_storage_size);
	}

	constexpr __StaticFunctionStorage(__StaticFunctionStorage const& storage)
	    : m_storage_size()
	    , m_storage() {
		m_storage_size = storage.m_storage_size;
		__builtin_memcpy(m_storage, storage.m_storage, storage.m_storage_size);
	}

	constexpr ~__StaticFunctionStorage() = default;

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
	constexpr KFunction()
	    : m_construct_func(nullptr)
	    , m_invoke_func(nullptr)
	    , m_destroy_func(nullptr)
	    , m_storage() {}

	template<typename Functor>
	requires LambdaCapturesOccupyASinglePointer<Functor>
	constexpr KFunction(Functor f)
	    : m_construct_func(reinterpret_cast<ConstructFunc>(construct_functor<Functor>))
	    , m_invoke_func(reinterpret_cast<InvokeFunc>(invoke_functor<Functor>))
	    , m_destroy_func(reinterpret_cast<DestroyFunc>(destroy_functor<Functor>))
	    , m_storage(f) {
		m_construct_func(m_storage.get(), reinterpret_cast<char*>(&f));
	}

	//  FIXME: This might break storage destruction, more tests needed
	constexpr KFunction(KFunction&&) = default;

	constexpr ~KFunction() { destruct(); }

	//  FIXME: Currently, no copying of functors allowed
	constexpr KFunction(KFunction const& f) = delete;
	constexpr KFunction& operator=(KFunction const&) = delete;

	constexpr Ret operator()(Args&&... args) { return m_invoke_func(m_storage.get(), gen::forward<Args>(args)...); }
};