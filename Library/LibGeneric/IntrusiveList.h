#pragma once

namespace gen {

	template<typename T>
	class IntrusiveNode {
	public:
		constexpr IntrusiveNode()
		    : m_prev()
		    , m_next() {}

		constexpr IntrusiveNode(IntrusiveNode const&) = delete;
		constexpr void operator=(IntrusiveNode const&) = delete;
	private:
		IntrusiveNode<T>* m_prev;
		T* m_next;

		template<typename S, IntrusiveNode<S> S::*>
		friend class IntrusiveList;
		template<typename S, IntrusiveNode<S> S::*>
		friend class IntrusiveIterator;
	};

	template<typename T, IntrusiveNode<T> T::*Link>
	class IntrusiveIterator {
	public:
		explicit IntrusiveIterator(IntrusiveNode<T>* current)
		    : current(current) {}

		constexpr T& operator*() { return *this->operator->(); }
		constexpr T* operator->() { return this->current->next; }
		constexpr bool operator==(IntrusiveIterator const& other) const { return this->current == other.current; }
		constexpr bool operator!=(IntrusiveIterator const& other) const { return !(*this == other); }

		constexpr IntrusiveIterator& operator++() {
			this->current = &(this->current->next->*Link);
			return *this;
		}

		constexpr IntrusiveIterator operator++(int) {
			IntrusiveIterator rc(*this);
			this->operator++();
			return rc;
		}

		constexpr IntrusiveIterator& operator--() {
			this->current = this->current->prev;
			return *this;
		}

		constexpr IntrusiveIterator operator--(int) {
			IntrusiveIterator rc(*this);
			this->operator--();
			return rc;
		}
	private:
		template<typename S, IntrusiveNode<S> S::*>
		friend class IntrusiveList;

		IntrusiveNode<T>* current;
	};

	template<typename T, IntrusiveNode<T> T::*Link>
	class IntrusiveList {
	public:
		constexpr IntrusiveList() { m_head.m_prev = &this->m_head; }

		constexpr IntrusiveIterator<T, Link> begin() { return IntrusiveIterator<T, Link> { &m_head }; }

		constexpr IntrusiveIterator<T, Link> end() { return IntrusiveIterator<T, Link> { &m_head.m_prev }; }

		constexpr T& front() { return *m_head.m_next; }

		constexpr T& back() { return *(m_head.m_prev->m_prev->m_next); }

		constexpr bool empty() const { return m_head.m_prev == &m_head; }
	private:
		IntrusiveNode<T> m_head;
	};
}
