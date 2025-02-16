#pragma once
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Move.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <string.h>

namespace gen {
	template<template<typename> class Alloc = gen::Allocator>
	class __FunctionStorage {
	public:
		typedef char* StoragePointer;
	private:
		typedef typename Alloc<char>::template rebind<char>::other StorageObjType;
		[[no_unique_address]] typename AllocatorTraits<StorageObjType>::allocator_type _buf_allocator;

		size_t m_storage_size;
		union Storage {
			StoragePointer heap_storage;
			char stack_storage[sizeof(StoragePointer)];
		};
		Storage m_storage;

		constexpr char* allocate_storage(size_t size) { return AllocatorTraits<StorageObjType>::allocate(size); }

		constexpr void free_storage(char* ptr, size_t size) { AllocatorTraits<StorageObjType>::deallocate(ptr, size); }

		constexpr bool is_heap_storage() const { return m_storage_size > sizeof(StoragePointer); }

		constexpr void initialize_storage(size_t size) {
			m_storage_size = size;
			if(is_heap_storage()) {
				m_storage.heap_storage = allocate_storage(size);
			} else {
				//  Functor is small enough - using the stack storage
			}
		}

		constexpr void destroy_storage() {
			if(is_heap_storage()) {
				free_storage(m_storage.heap_storage, m_storage_size);
			}
		}

		constexpr void move_storage(__FunctionStorage&& storage) {
			if(storage.is_heap_storage()) {
				//  Copy heap storage pointer and null it out on the source to prevent double-frees
				m_storage.heap_storage = storage.m_storage.heap_storage;
				storage.m_storage.heap_storage = nullptr;
			} else {
				//  Just copy the bytes from the built-in storage
				memcpy(m_storage.stack_storage, storage.m_storage.stack_storage, storage.m_storage_size);
			}
		}

		constexpr void clone_storage(__FunctionStorage const& storage) {
			//  FIXME: Very thread-unsafe
			initialize_storage(storage.m_storage_size);
			if(storage.is_heap_storage()) {
				memcpy(m_storage.heap_storage, storage.m_storage.heap_storage, storage.m_storage_size);
			} else {
				memcpy(m_storage.stack_storage, storage.m_storage.stack_storage, storage.m_storage_size);
			}
		}
	public:
		constexpr __FunctionStorage()
		    : m_storage_size(0)
		    , m_storage() {}

		constexpr explicit __FunctionStorage(size_t storage_size)
		    : m_storage_size(storage_size)
		    , m_storage() {
			initialize_storage(storage_size);
		}

		constexpr __FunctionStorage(__FunctionStorage&& storage) noexcept
		    : m_storage_size(storage.m_storage_size)
		    , m_storage() {
			move_storage(storage);
		}

		constexpr __FunctionStorage(__FunctionStorage const& storage)
		    : m_storage_size()
		    , m_storage() {
			clone_storage(storage);
		}

		constexpr ~__FunctionStorage() { destroy_storage(); }

		constexpr StoragePointer get() {
			if(is_heap_storage()) {
				return m_storage.heap_storage;
			} else {
				return m_storage.stack_storage;
			}
		}

		constexpr size_t size() const { return m_storage_size; }
	};

	template<typename T>
	class Function;

	template<typename Ret, typename... Args>
	class Function<Ret(Args...)> {
		typedef Ret (*InvokeFunc)(char*, Args&&...);
		typedef void (*ConstructFunc)(char*, char*);
		typedef void (*DestroyFunc)(char*);

		ConstructFunc m_construct_func;
		InvokeFunc m_invoke_func;
		DestroyFunc m_destroy_func;

		__FunctionStorage<gen::Allocator> m_storage;

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
		constexpr Function()
		    : m_construct_func(nullptr)
		    , m_invoke_func(nullptr)
		    , m_destroy_func(nullptr)
		    , m_storage(0) {}

		template<typename Functor>
		constexpr Function(Functor f)
		    : m_construct_func(reinterpret_cast<ConstructFunc>(construct_functor<Functor>))
		    , m_invoke_func(reinterpret_cast<InvokeFunc>(invoke_functor<Functor>))
		    , m_destroy_func(reinterpret_cast<DestroyFunc>(destroy_functor<Functor>))
		    , m_storage(sizeof(Functor)) {
			m_construct_func(m_storage.get(), reinterpret_cast<char*>(&f));
		}

		constexpr ~Function() { destruct(); }

		//  FIXME: Currently, no copying of functors allowed
		constexpr Function(Function const& f) = delete;
		constexpr Function& operator=(Function const&) = delete;

		constexpr Ret operator()(Args&&... args) { return m_invoke_func(m_storage.get(), gen::forward<Args>(args)...); }
	};
}