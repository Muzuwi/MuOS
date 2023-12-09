#include "Core/Object/Tree.hpp"
#include <LibGeneric/List.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <Locks/KMutex.hpp>
#include "Core/Error/Error.hpp"
#include "LibGeneric/Algorithm.hpp"
#include "LibGeneric/Move.hpp"

static gen::List<core::obj::KObject*> s_kernel_objects;
static KMutex s_tree_lock;

static bool tree_contains_object(core::obj::KObject* object) {
	const auto result = gen::find_if(s_kernel_objects, [object](core::obj::KObject* other) { return object == other; });
	return result != s_kernel_objects.end();
}

static core::Error tree_register_object(core::obj::KObject* object) {
	if(!object) {
		return core::Error::InvalidArgument;
	}
	s_kernel_objects.push_back(object);
	return core::Error::Ok;
}

/**
 * Register an object in the kernel object tree.
 */
core::Error core::obj::attach(KObject* object) {
	gen::LockGuard g { s_tree_lock };

	if(!object) {
		return core::Error::InvalidArgument;
	}
	if(tree_contains_object(object)) {
		return core::Error::InvalidArgument;
	}
	return tree_register_object(object);
}

/**
 * Calls the provided callback for each object in the kernel
 * object tree that has the provided type.
 *
 * All kernel tree object functions lock the tree, and the callback is called
 * while the object tree lock is held. The callback is prohibited from performing
 * any operations on the tree, as otherwise a deadlock will occur.
 */
core::Error core::obj::for_each_object_of_type(ObjectType t, FindObjectCallback f) {
	gen::LockGuard g { s_tree_lock };

	for(auto* object : s_kernel_objects) {
		if(!object) {
			continue;
		}
		if(object->type() != t) {
			continue;
		}
		f(gen::move(object));
	}
	return core::Error::Ok;
}
