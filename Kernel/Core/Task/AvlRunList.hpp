#pragma once
#include <Core/Error/Error.hpp>
#include <Core/Task/Task.hpp>
#include <LibGeneric/AVL.hpp>
#include <LibGeneric/Spinlock.hpp>

namespace core::sched {
	/**	Implementation of IRunList utilizing an intrusive AVL tree
	 *
	 *	Changes are required to core::task::Task to store the node intrusively.
	 */
	class AvlRunList {
	public:
		core::Error add(core::task::Task*);
		core::Error remove(core::task::Task*);
		core::task::Task* next();
	private:
		struct NodeComparer {
			inline bool compare(gen::AvlNode& a, gen::AvlNode& b) {
				core::task::Task* lhs = container_of(&a, core::task::Task, runlist_node);
				core::task::Task* rhs = container_of(&b, core::task::Task, runlist_node);
				return task::is_higher_priority(lhs->priority, rhs->priority);
			}
		};
		struct NodeFetcher {
			constexpr gen::AvlNode& operator()(core::task::Task& v) { return v.runlist_node; }
		};
		using RunListTree = gen::AvlTree<core::task::Task, NodeComparer, NodeFetcher>;

		RunListTree m_tree;
		gen::Spinlock m_lock;
	};
}
