#pragma once
#include <stdint.h>
#include <stddef.h>

namespace gen {
	class __SharedCount {
	public:
		typedef size_t _refcount_t;
	private:
		_refcount_t* m_ref_count;

		constexpr void _storage_free() {
			delete m_ref_count;
			m_ref_count = nullptr;
		}

		constexpr void _add_ref() {
			if(!m_ref_count) return;

			++(*m_ref_count);
		}

		constexpr void _release_ref() {
			if(!m_ref_count) return;

			--(*m_ref_count);

			if(*m_ref_count == 0)
				_storage_free();
		}
	public:
		constexpr __SharedCount(_refcount_t* p) noexcept
		: m_ref_count(p) {}

		explicit __SharedCount(const __SharedCount& ptr) noexcept
		: m_ref_count(ptr.m_ref_count) {
			_add_ref();
		}

		__SharedCount& operator=(const __SharedCount& c) {
			if(&c == this) return *this;

			_release_ref();
			m_ref_count = c.m_ref_count;
			_add_ref();

			return *this;
		}

		~__SharedCount() noexcept {
			_release_ref();
		}

		_refcount_t _use_count() const {
			return ((m_ref_count) ? (*m_ref_count) : 0);
		}

		void _swap(__SharedCount& v) {
			auto* tmp = v.m_ref_count;
			v.m_ref_count = m_ref_count;
			m_ref_count = tmp;
		}
	};


	template<class T>
	class SharedPtr {
		T* m_ptr;
		__SharedCount m_ref_count;

		constexpr void _on_destruct() {
			if(m_ref_count._use_count() == 1) {
				delete m_ptr;
				m_ptr = nullptr;
			}
		}

		inline __SharedCount::_refcount_t* _alloc_refcount() {
			return new __SharedCount::_refcount_t(1);
		}
	public:
		constexpr SharedPtr() noexcept
		: m_ptr(nullptr), m_ref_count(nullptr) {}

		explicit constexpr SharedPtr(nullptr_t)
		: SharedPtr() {}

		explicit SharedPtr(T* ptr) noexcept
		: m_ptr(ptr), m_ref_count(_alloc_refcount()) {}

		SharedPtr(const SharedPtr& ptr) noexcept
		: m_ptr(ptr.m_ptr), m_ref_count(ptr.m_ref_count) {}

		~SharedPtr() {
			//  Free when we're the only reference left
			_on_destruct();
		}

		SharedPtr& operator=(const SharedPtr& ptr) noexcept {
			if(&ptr == this) return *this;

			_on_destruct();
			m_ptr = ptr.m_ptr;
			m_ref_count = ptr.m_ref_count;

			return *this;
		}

		size_t use_count() const {
			return m_ref_count._use_count();
		}

		T* get() const noexcept {
			return m_ptr;
		}

		T& operator*() const noexcept {
			return *m_ptr;
		}

		T* operator->() const noexcept {
			return m_ptr;
		}

		operator bool() {
			return (m_ptr != nullptr);
		}

		void reset() noexcept {
			_on_destruct();
			m_ptr = nullptr;
			m_ref_count = nullptr;
		}

		template<class Y>
		void reset(Y* ptr) {
			_on_destruct();
			m_ptr = ptr;
			m_ref_count = _alloc_refcount();
		}

		void swap(SharedPtr& r) noexcept {
			auto* tmp = r.m_ptr;
			r.m_ptr = m_ptr;
			m_ptr = tmp;

			m_ref_count._swap(r.m_ref_count);
		}
	};

	template<class T, class... Args>
	SharedPtr<T> make_shared(Args&&... args) {
		return SharedPtr<T>(new T(args...));
	}
}

