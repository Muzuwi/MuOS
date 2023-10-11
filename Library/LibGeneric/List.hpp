#pragma once
#include <LibGeneric/Allocator.hpp>
#include <LibGeneric/Iterator.hpp>
#include <stddef.h>

namespace gen {

	template<class T, template<class> class Alloc>
	class _BidirectionalIterator_List;
	template<class T, template<class> class Alloc>
	class _BidirectionalIterator_List_Const;

	/*
	 *  Class implementing a doubly-linked list
	 */
	template<class T, template<class> class Alloc = gen::Allocator>
	class List {
	protected:
		friend class _BidirectionalIterator_List<T, Alloc>;
		friend class _BidirectionalIterator_List_Const<T, Alloc>;

		class Node {
		protected:
			friend class List;

			friend class _BidirectionalIterator_List<T, Alloc>;
			friend class _BidirectionalIterator_List_Const<T, Alloc>;

			Node* m_next;
			Node* m_prev;

			Node() {
				m_next = this;
				m_prev = this;
			}
		public:
			Node(Node* prev, Node* next) {
				m_next = next;
				m_prev = prev;
			}
		};

		class DataNode : public Node {
			friend List;

			T m_data;
		public:
			DataNode(T val)
			    : Node(nullptr, nullptr)
			    , m_data(val) {}

			T& value() { return m_data; }

			const T& value() const { return m_data; }
		};

		static void _hook(Node* a, Node* b) {
			if(!a || !b)
				return;

			b->m_next = a->m_next;
			a->m_next->m_prev = b;
			a->m_next = b;
			b->m_prev = a;
		}

		static void _unhook(Node* a) {
			if(!a)
				return;

			a->m_prev->m_next = a->m_next;
			a->m_next->m_prev = a->m_prev;
		}
	public:
		typedef _BidirectionalIterator_List<T, Alloc> iterator;
		typedef _BidirectionalIterator_List_Const<T, Alloc> const_iterator;
	protected:
		typedef typename Alloc<T>::template rebind<DataNode>::other NodeAllocType;
		typename AllocatorTraits<NodeAllocType>::allocator_type _allocator;

		DataNode* _create_datanode(const T& data) {
			auto* node = AllocatorTraits<NodeAllocType>::allocate(1);
			AllocatorTraits<NodeAllocType>::construct(_allocator, node, data);
			return node;
		}

		void _destroy_node(Node* node) {
			AllocatorTraits<NodeAllocType>::destroy(_allocator, node);
			AllocatorTraits<NodeAllocType>::deallocate(reinterpret_cast<DataNode*>(node), 1);
		}
	private:
		Node m_head;
	public:
		List() noexcept = default;

		List(const List& rhs) noexcept {
			if(rhs.empty())
				return;

			Node* cur = rhs.m_head.m_next;
			while(cur && cur != &rhs.m_head) {
				this->push_back(reinterpret_cast<DataNode*>(cur)->m_data);
				cur = cur->m_next;
			}
		}

		~List() { this->clear(); }

		/*
		 *  Inserts a copy of the given value at the beginning of the list
		 */
		void push_front(const T& data) {
			auto* node = _create_datanode(data);
			_hook(&m_head, node);
		}

		/*
		 *  Inserts a copy of the given value at the end of the list
		 */
		void push_back(const T& data) {
			auto* node = _create_datanode(data);
			_hook(m_head.m_prev, node);
		}

		/*
		 *  Removes the front element from the list
		 */
		void pop_front() {
			if(empty())
				return;

			auto* node = m_head.m_next;
			_unhook(node);
			_destroy_node(node);
		}

		/*
		 *  Removes the last element from the list
		 */
		void pop_back() {
			if(empty())
				return;

			auto* node = m_head.m_prev;
			_unhook(node);
			_destroy_node(node);
		}

		/*
		 *  Returns a reference/const-reference to the front element of the list
		 *  Calling this function on an empty list is undefined
		 */
		T& front() { return reinterpret_cast<DataNode*>(m_head.m_next)->value(); }

		const T& front() const { return reinterpret_cast<DataNode*>(m_head.m_next)->value(); }

		/*
		 *  Returns a reference/const-reference to the back element of the list
		 *  Calling this function on an empty list is undefined
		 */
		T& back() { return reinterpret_cast<DataNode*>(m_head.m_prev)->value(); }

		const T& back() const { return reinterpret_cast<DataNode*>(m_head.m_prev)->value(); }

		/*
		 *  Returns the begin iterator of the list
		 */
		iterator begin() { return iterator(m_head.m_next); }

		const_iterator begin() const { return const_iterator(m_head.m_next); }

		/*
		 *  Returns the end iterator of the list
		 *  This is a placeholder iterator, accessing it is undefined behavior
		 */
		iterator end() { return iterator(&m_head); }

		const_iterator end() const { return const_iterator(&m_head); }

		/*
		 *  Clears all elements from the list
		 */
		void clear() {
			if(empty())
				return;

			Node* cur = m_head.m_next;
			while(cur != &m_head && cur) {
				auto* temp = cur->m_next;
				_destroy_node(cur);
				cur = temp;
			}

			m_head.m_next = &m_head;
			m_head.m_prev = &m_head;
		}

		/*
		 *  Checks whether the list is empty
		 */
		bool empty() const { return m_head.m_next == &m_head && m_head.m_prev == &m_head; }

		/*
		 *  Erases a node given by an iterator
		 */
		void erase(iterator it) {
			//  Safeguard
			if(it.m_it == &m_head)
				return;
			_unhook(it.m_it);

			_destroy_node(it.m_it);
		}

		/*
		 *  Returns the amount of elements in the list.
		 *  Warning! This function is linear-complexity, and not constant
		 */
		size_t size() const {
			size_t s = 0;

			Node* cur = m_head.m_next;
			while(cur && cur != &m_head) {
				cur = cur->m_next;
				++s;
			}

			return s;
		}

		/*
		 *  Inserts a value before position specified by an iterator
		 */
		iterator insert(iterator pos, const T& v) {
			auto* node = _create_datanode(v);
			_hook(pos.m_it->m_prev, node);
			return iterator { node };
		}
	};

	template<class T, template<class> class Alloc>
	class _BidirectionalIterator_List : Iterator<T> {
		friend class gen::List<T, Alloc>;

		typedef gen::List<T, Alloc> list_type;
		typedef typename gen::List<T, Alloc>::Node node_type;
		typedef typename gen::List<T, Alloc>::DataNode datanode_type;
		typedef T value_type;

		typedef _BidirectionalIterator_List<T, Alloc> iter_type;

		node_type* m_it;
	public:
		_BidirectionalIterator_List(const list_type& list) { m_it = list.m_head.m_next; }

		_BidirectionalIterator_List(node_type* node) { m_it = node; }

		iter_type& operator++() {
			m_it = m_it->m_next;
			return *this;
		}

		iter_type operator++(int) {
			iter_type temp { *this };
			m_it = m_it->m_next;
			return temp;
		}

		iter_type& operator--() {
			m_it = m_it->m_prev;
			return *this;
		}

		iter_type operator--(int) {
			iter_type temp { *this };
			m_it = m_it->m_prev;
			return temp;
		}

		bool operator!=(const iter_type& rhs) const { return m_it != rhs.m_it; }

		bool operator==(const iter_type& rhs) const { return m_it == rhs.m_it; }

		value_type& operator*() { return reinterpret_cast<datanode_type*>(m_it)->value(); }

		const value_type& operator*() const { return reinterpret_cast<datanode_type const*>(m_it)->value(); }
	};

	template<class T, template<class> class Alloc>
	class _BidirectionalIterator_List_Const : Iterator<T> {
		friend class gen::List<T, Alloc>;

		typedef gen::List<T, Alloc> list_type;
		typedef typename gen::List<T, Alloc>::Node node_type;
		typedef typename gen::List<T, Alloc>::DataNode datanode_type;
		typedef T value_type;

		typedef _BidirectionalIterator_List_Const<T, Alloc> iter_type;

		node_type const* m_it;
	public:
		_BidirectionalIterator_List_Const(const list_type& list) { m_it = list.m_head.m_next; }

		_BidirectionalIterator_List_Const(const node_type* node) { m_it = node; }

		_BidirectionalIterator_List_Const& operator++() {
			m_it = m_it->m_next;
			return *this;
		}

		iter_type operator++(int) {
			iter_type temp { m_it };
			m_it = m_it->m_next;
			return temp;
		}

		iter_type& operator--() {
			m_it = m_it->m_prev;
			return *this;
		}

		iter_type operator--(int) {
			iter_type temp { m_it };
			m_it = m_it->m_prev;
			return temp;
		}

		bool operator!=(const iter_type& rhs) const { return m_it != rhs.m_it; }

		bool operator==(const iter_type& rhs) const { return m_it == rhs.m_it; }

		const value_type& operator*() const { return reinterpret_cast<datanode_type const*>(m_it)->value(); }
	};

}
