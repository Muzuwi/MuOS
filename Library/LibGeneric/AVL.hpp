#pragma once
#include <LibGeneric/Traits.hpp>
#include <stddef.h>
#include <stdint.h>

namespace gen {
	using AvlTreeHandle = void*;
	class AvlNode;

	enum class TreeIteration {
		PreOrder,
		InOrder,
		PostOrder,
	};

	template<typename T, typename Comparer>
	concept NodeValueComparator = requires(Comparer node_comparer, AvlNode& left, AvlNode& right) {
		{ node_comparer.compare(left, right) } -> gen::MustBeSameType<bool>;
	};

	template<typename T, typename Fetcher>
	concept NodeDataFetcher = requires(Fetcher node_data_fetcher, T tree_type) {
		{ node_data_fetcher(tree_type) } -> gen::MustBeSameType<AvlNode&>;
	};

	template<typename T, typename NodeComparator, typename NodeDataStorer>
	requires NodeValueComparator<T, NodeComparator> && NodeDataFetcher<T, NodeDataStorer>
	class AvlTree;

	class AvlNode {
	public:
		constexpr AvlNode() = default;

		[[nodiscard]] constexpr AvlNode* parent() const { return m_parent; }

		[[nodiscard]] constexpr AvlNode* left() const { return m_left; }

		[[nodiscard]] constexpr AvlNode* right() const { return m_right; }
	private:
		constexpr AvlNode(AvlNode* parent, AvlNode* left, AvlNode* right, AvlTreeHandle tree)
		    : m_tree(tree)
		    , m_parent(parent)
		    , m_left(left)
		    , m_right(right) {};

		constexpr AvlNode* rotate_left() {
			auto* B = m_right;
			m_right = B->m_left;
			B->m_left = this;
			auto* our_parent = m_parent;
			m_parent = B;
			B->m_parent = our_parent;

			const auto A_coeff = m_balance_coeff;
			const auto B_coeff = B->m_balance_coeff;
			if(B_coeff <= 0) {
				if(A_coeff >= 1) {
					B->m_balance_coeff = B_coeff - 1;
				} else {
					B->m_balance_coeff = A_coeff + B_coeff - 2;
				}
				m_balance_coeff = A_coeff - 1;
			} else {
				if(A_coeff <= B_coeff) {
					B->m_balance_coeff = A_coeff - 2;
				} else {
					B->m_balance_coeff = B_coeff - 1;
				}
				m_balance_coeff = A_coeff - B_coeff - 1;
			}
			return B;
		}

		constexpr AvlNode* rotate_right() {
			auto* B = m_left;
			m_left = B->m_right;
			B->m_right = this;
			auto* our_parent = m_parent;
			m_parent = B;
			B->m_parent = our_parent;

			const auto A_coeff = m_balance_coeff;
			const auto B_coeff = B->m_balance_coeff;
			if(B_coeff <= 0) {
				if(B_coeff > A_coeff)
					B->m_balance_coeff = B_coeff + 1;
				else
					B->m_balance_coeff = A_coeff + 2;
				m_balance_coeff = A_coeff - B_coeff + 1;
			} else {
				if(A_coeff <= -1)
					B->m_balance_coeff = B_coeff + 1;
				else
					B->m_balance_coeff = A_coeff + B_coeff + 2;
				m_balance_coeff = A_coeff + 1;
			}
			return B;
		}

		constexpr AvlNode* rebalance() {
			if(m_balance_coeff < -1) {
				//  left-heavy
				if(m_left->m_balance_coeff <= 0) {
					//  left/left
					return rotate_right();
				} else {
					//  left/right
					m_left = m_left->rotate_left();
					return rotate_right();
				}
			} else if(m_balance_coeff > 1) {
				//  right-heavy
				if(m_right->m_balance_coeff >= 0) {
					//  right/right
					return rotate_left();
				} else {
					//  right/left
					m_right = m_right->rotate_right();
					return rotate_left();
				}
			} else {
				//  balanced
				return this;
			}
		}

		AvlTreeHandle m_tree { nullptr };
		AvlNode* m_parent { nullptr };
		AvlNode* m_left { nullptr };
		AvlNode* m_right { nullptr };
		int m_balance_coeff { 0 };

		template<typename T, typename NodeComparator, typename NodeDataStorer>
		requires NodeValueComparator<T, NodeComparator> && NodeDataFetcher<T, NodeDataStorer>
		friend class AvlTree;
	};

	/**	Iterator over an AVL tree
	 *
	 * 	This is a wrapper that fulfills the requirement for an iterator
	 * 	to be usable with for-range constructs. The iterator additionally
	 *	contains the current iteration order to allow it to be used with
	 *	different tree orderings (pre/in/post).
	 */
	template<TreeIteration Order>
	class AvlNodeIterator {
	public:
		/**	Construct an iterator over an AVL tree with root node specified by `start`.
		 */
		constexpr AvlNodeIterator(AvlNode* start)
		    : m_root(start)
		    , m_current(forward_to_initial_node(start)) {}

		/**	Construct an empty / end iterator.
		 */
		constexpr AvlNodeIterator()
		    : m_root(nullptr)
		    , m_current(nullptr) {}

		[[nodiscard]] constexpr AvlNodeIterator begin() const { return { m_root }; }

		[[nodiscard]] constexpr AvlNodeIterator end() const { return {}; }

		constexpr AvlNodeIterator& operator++() {
			//  Nothing to do - already at end()
			if(!m_current) {
				return *this;
			}

			if constexpr(Order == TreeIteration::PreOrder) {
				if(m_current->left()) {
					m_current = m_current->left();
				} else if(m_current->right()) {
					m_current = m_current->right();
				} else {
					//  Climb up until we find a node with an unvisited right child
					AvlNode* parent = m_current->parent();
					while(parent && (parent->right() == m_current || !parent->right())) {
						m_current = parent;
						parent = parent->parent();
					}
					m_current = parent ? parent->right() : nullptr;
				}
			} else if constexpr(Order == TreeIteration::InOrder) {
				if(m_current->right()) {
					//  Move to the leftmost node of the right subtree
					m_current = m_current->right();
					while(m_current->left()) {
						m_current = m_current->left();
					}
				} else {
					//  Backtrack to the parent
					auto* parent = m_current->parent();
					while(parent && m_current == parent->right()) {
						m_current = parent;
						parent = parent->parent();
					}
					m_current = parent;
				}
			} else if constexpr(Order == TreeIteration::PostOrder) {
				AvlNode* parent = m_current->parent();
				if(parent && m_current == parent->left() && parent->right()) {
					//  If moving up from the left child and there's a right child, go to it
					m_current = parent->right();
					while(m_current->left() || m_current->right()) {
						m_current = m_current->left() ? m_current->left() : m_current->right();
					}
				} else {
					//  Move to the parent
					m_current = parent;
				}
			}
			//  If we're finished, mark the iterator as completed
			if(!m_current) {
				m_root = nullptr;
			}
			return *this;
		}

		constexpr AvlNodeIterator operator++(int) {
			auto v = *this;
			operator++();
			return v;
		}

		constexpr AvlNode* operator*() { return m_current; }

		constexpr bool operator!=(AvlNodeIterator const& rhs) {
			//  NOTE: Order is irrelevant, nullptr is used as the
			//  sentinel value for end-of-iteration.
			return (m_current != rhs.m_current) || (m_root != rhs.m_root);
		}
	private:
		static constexpr AvlNode* forward_to_initial_node(AvlNode* start) {
			if constexpr(Order == TreeIteration::PreOrder) {
				return start;
			} else if constexpr(Order == TreeIteration::InOrder) {
				while(start && start->left()) {
					start = start->left();
				}
				return start;
			} else if constexpr(Order == TreeIteration::PostOrder) {
				while(start && start->left()) {
					start = start->left();
				}
				while(start && start->right()) {
					start = start->right();
				}
				return start;
			}
		}

		AvlNode* m_root;
		AvlNode* m_current;
	};

	template<typename T, typename NodeComparator, typename NodeDataStorer>
	requires NodeValueComparator<T, NodeComparator> && NodeDataFetcher<T, NodeDataStorer>
	class AvlTree {
	public:
		AvlTree() = default;
		~AvlTree() = default;

		bool add(T& data) {
			auto& node = getnode(data);
			if(!node_bind(&node)) {
				return false;
			}

			if(!m_root) {
				m_root = &node;
				m_root->m_parent = nullptr;
				m_root->m_left = nullptr;
				m_root->m_right = nullptr;
				m_root->m_balance_coeff = 0;
				(void)node_bind(m_root);
				return true;
			}
			node_add(m_root, data);
			return true;
		}

		bool remove(T& data) {
			if(!contains(data)) {
				return false;
			}
			auto& node = getnode(data);
			node_remove(&node);
			return true;
		}

		bool contains(T& data) const { return node_is_bound(&getnode(data)); }

		[[nodiscard]] constexpr size_t size() const { return m_size; }

		void clear() { node_remove(m_root); }

		/**	Iterate over the tree in a given order.
		 *
		 * 	Returns an iterator that can be used to iterate over
		 * 	all elements in the tree with the specified tree iteration
		 * 	order.
		 */
		template<TreeIteration Order>
		constexpr AvlNodeIterator<Order> iterate() {
			return AvlNodeIterator<Order>(m_root);
		}
	private:
		constexpr gen::AvlNode& getnode(T& data) const { return m_data_fetcher(data); }

		/**	Bind a new node to this tree
		 *
		 *	If the node is already part of a different tree, or the
		 *	node is already part of this tree, this will return false.
		 *
		 * 	Binding ensures that nodes from a different tree cannot be
		 *	used on this tree.
		 */
		bool node_bind(AvlNode* ptr) {
			if(!ptr) {
				return false;
			}
			if(ptr->m_tree == nullptr) {
				m_size += 1;
				ptr->m_tree = this;
				return true;
			}
			return false;
		}

		/**	Unbind a node from this tree
		 *
		 * 	If the node is not part of this tree, this will
		 * 	return false.
		 */
		bool node_unbind(AvlNode* ptr) {
			if(!ptr) {
				return false;
			}
			if(ptr->m_tree == this) {
				m_size -= 1;
				ptr->m_tree = nullptr;
				return true;
			}
			return false;
		}

		/**	Check if a given node is bound to this tree
		 */
		bool node_is_bound(AvlNode const* ptr) const {
			if(!ptr) {
				return false;
			}
			return ptr->m_tree == this;
		}

		/**	Add a new data node, starting the search at a given node pointer
		 */
		bool node_add(AvlNode* ptr, T& data) {
			if(!ptr) {
				return false;
			}
			AvlNode& new_node = getnode(data);
			AvlNode* current = ptr;

			//  First, traverse the tree and find a parent for the new child
			while(current) {
				const bool value_less_than_current = m_comparator.compare(new_node, *current);
				if(value_less_than_current) {
					if(current->m_left) {
						current = current->m_left;
					} else {
						current->m_left = &new_node;
						current->m_left->m_parent = current;
						current->m_balance_coeff -= 1;
						break;
					}
				} else {
					if(current->m_right) {
						current = current->m_right;
					} else {
						current->m_right = &new_node;
						current->m_right->m_parent = current;
						current->m_balance_coeff += 1;
						break;
					}
				}
			}

			//  At this point, `current` points to the parent of the new child
			//  Backtrack up the tree and rebalance all parents
			AvlNode* parent = current;
			while(parent) {
				parent = parent->rebalance();
				parent = parent->parent();
			}
			return true;
		}

		void shift_nodes(AvlNode* u, AvlNode* v) {
			if(!u->m_parent) {
				m_root = v;
			} else if(u == u->m_parent->m_left) {
				u->m_parent->m_left = v;
			} else {
				u->m_parent->m_right = v;
			}
			if(v) {
				v->m_parent = u->m_parent;
			}
		}

		/**	Remove a given node from the tree
		 */
		void node_remove(AvlNode* z) {
			AvlNode* ap;
			if(!z->m_left) {
				shift_nodes(z, z->m_right);
				ap = z->m_parent;
			} else if(!z->m_right) {
				shift_nodes(z, z->m_left);
				ap = z->m_parent;
			} else {
				//  `ptr` has 2 children, find the in-order successor of `ptr`
				AvlNode* y = z->m_right;
				while(y->m_left) {
					y = y->m_left;
				}
				if(y->m_parent != z) {
					shift_nodes(y, y->m_right);
				}
				shift_nodes(z, y);
				y->m_left = z->m_left;
				y->m_left->m_parent = y;
				ap = y->m_parent;
			}

			//  Backtrack up the tree and rebalance all parents
			while(ap) {
				ap = ap->rebalance();
				ap = ap->m_parent;
			}
			//  unbind the now-deleted node from the tree
			node_unbind(z);
			//  Reset node to the default/null state, otherwise the
			//  node would contain stale data when being re-added
			//  to the tree.
			*z = AvlNode {};
		}

		AvlNode* m_root { nullptr };
		size_t m_size {};
		[[no_unique_address]] mutable NodeComparator m_comparator {};
		[[no_unique_address]] mutable NodeDataStorer m_data_fetcher {};
	};
}