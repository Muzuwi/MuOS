#pragma once
#include <LibGeneric/BidirectionalIterator.hpp>

namespace gen {
	/*
	 *  Class implementing a doubly-linked list
	 */
	template<class T>
	class List {
	protected:
		friend class BidirectionalIterator<List<T>>;

		class Node {
		protected:
			friend class List;

			friend class BidirectionalIterator<List<T>>;

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
			DataNode(T val) : Node(nullptr, nullptr), m_data(val) {}

			T& value() {
				return m_data;
			}

			T& value() const {
				return m_data;
			}
		};

		static void _hook(Node* a, Node* b) {
			if (!a || !b) return;

			b->m_next = a->m_next;
			a->m_next->m_prev = b;
			a->m_next = b;
			b->m_prev = a;
		}

		static void _unhook(Node* a) {
			if (!a) return;

			a->m_prev->m_next = a->m_next;
			a->m_next->m_prev = a->m_prev;
		}

		typedef BidirectionalIterator<List<T>> iterator;
		typedef const BidirectionalIterator<List<T>> const_iterator;

	private:
		Node m_head;
	public:
		List() {}

		List(const List& rhs) {
			if (rhs.empty()) return;

			Node* cur = rhs.m_head.m_next;
			while (cur && cur != &rhs.m_head) {
				this->push_back(reinterpret_cast<DataNode*>(cur)->m_data);
				cur = cur->m_next;
			}
		}

		~List() {
			this->clear();
		}

		/*
		 *  Inserts a copy of the given value at the beginning of the list
		 */
		void push_front(const T& data) {
			auto* node = new DataNode(data);
			_hook(&m_head, node);
		}

		/*
		 *  Inserts a copy of the given value at the end of the list
		 */
		void push_back(const T& data) {
			auto* node = new DataNode(data);
			_hook(m_head.m_prev, node);
		}

		/*
		 *  Removes the front element from the list
		 */
		void pop_front() {
			if (empty()) return;

			auto* node = m_head.m_next;
			_unhook(node);
			delete reinterpret_cast<DataNode*>(node);
		}

		/*
		 *  Removes the last element from the list
		 */
		void pop_back() {
			if (empty()) return;

			auto* node = m_head.m_prev;
			_unhook(node);
			delete reinterpret_cast<DataNode*>(node);
		}

		/*
		 *  Returns a reference/const-reference to the front element of the list
		 *  Calling this function on an empty list is undefined
		 */
		T& front() {
			return reinterpret_cast<DataNode*>(m_head.m_next)->value();
		}

		const T& front() const {
			return reinterpret_cast<DataNode*>(m_head.m_next)->value();
		}

		/*
		 *  Returns a reference/const-reference to the back element of the list
		 *  Calling this function on an empty list is undefined
		 */
		T& back() {
			return reinterpret_cast<DataNode*>(m_head.m_prev)->value();
		}

		const T& back() const {
			return reinterpret_cast<DataNode*>(m_head.m_prev)->value();
		}

		/*
		 *  Returns the begin iterator of the list
		 */
		iterator begin() {
			return iterator(m_head.m_next);
		}

		const_iterator begin() const {
			return const_iterator(m_head.m_next);
		}

		/*
		 *  Returns the end iterator of the list
		 *  This is a placeholder iterator, accessing it is undefined behavior
		 */
		iterator end() {
			return iterator(&m_head);
		}

		const_iterator end() const {
			return const_iterator(&m_head);
		}

		/*
		 *  Clears all elements from the list
		 */
		void clear() {
			if (empty()) return;

			Node* cur = m_head.m_next;
			while (cur != &m_head && cur) {
				auto* temp = cur->m_next;
				delete reinterpret_cast<DataNode*>(cur);
				cur = temp;
			}

			m_head.m_next = &m_head;
			m_head.m_prev = &m_head;
		}

		/*
		 *  Checks whether the list is empty
		 */
		bool empty() const {
			return m_head.m_next == m_head.m_prev;
		}

		/*
		 *  Erases a node given by an iterator
		 */
		void erase(iterator it) {
			//  Safeguard
			if (it.m_it == &m_head) return;
			_unhook(it.m_it);
			delete reinterpret_cast<DataNode*>(it.m_it);
		}

		/*
		 *  Returns the amount of elements in the list.
		 *  Warning! This function is linear-complexity, and not constant
		 */
		size_t size() const {
			size_t s = 0;

			Node* cur = m_head.m_next;
			while (cur && cur != &m_head) {
				cur = cur->m_next;
				++s;
			}

			return s;
		}

		/*
		 *  Inserts a value before position specified by an iterator
		 */
		iterator insert(iterator pos, const T& v) {
			auto* node = new DataNode(v);
			_hook(pos.m_it->m_prev, node);
			return iterator {node};
		}

	};

	/*
	 *  Iterator specialization for linked lists
	 */
	template<class T>
	class BidirectionalIterator<gen::List<T>> : Iterator<gen::List<T>> {
		friend class gen::List<T>;

		typedef gen::List<T> list_type;
		typedef typename gen::List<T>::Node* node_type;
		typedef typename gen::List<T>::DataNode* datanode_type;
		node_type m_it;
	public:
		BidirectionalIterator(const list_type& list) {
			m_it = list.m_head.m_next;
		}

		BidirectionalIterator(node_type node) {
			m_it = node;
		}

		BidirectionalIterator& operator++() {
			m_it = m_it->m_next;
			return *this;
		}

		BidirectionalIterator operator++(int) {
			BidirectionalIterator temp {m_it};
			m_it = m_it->m_next;
			return temp;
		}

		BidirectionalIterator& operator--() {
			m_it = m_it->m_prev;
			return *this;
		}

		BidirectionalIterator operator--(int) {
			BidirectionalIterator temp {m_it};
			m_it = m_it->m_prev;
			return temp;
		}

		bool operator==(const BidirectionalIterator& rhs) const {
			return m_it == rhs.m_it;
		}

		bool operator!=(const BidirectionalIterator& rhs) const {
			return m_it != rhs.m_it;
		}

		typename Iterator<T>::value_type& operator*() {
			return reinterpret_cast<datanode_type>(m_it)->value();
		}
	};
}


