#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdlib>
#include <LibGeneric/AVL.hpp>
#include <memory>
#include <numeric>
#include <vector>

#define container_of(ptr, type, member)                     \
	({                                                      \
		const decltype(((type*)0)->member)* __mptr = (ptr); \
		(type*)((char*)__mptr - offsetof(type, member));    \
	})

struct MyStruct {
	int value;
	gen::AvlNode node;
};
struct NodeFetcher {
	constexpr gen::AvlNode& operator()(MyStruct& v) { return v.node; }
};
struct NodeComparer {
	inline bool compare(gen::AvlNode& a, gen::AvlNode& b) {
		MyStruct* lhs = container_of(&a, MyStruct, node);
		MyStruct* rhs = container_of(&b, MyStruct, node);
		return lhs->value < rhs->value;
	}
};

TEST_CASE("gen::AvlTree", "[algo]") {
	SECTION("simple construction compiles") {
		REQUIRE(requires {
			{ gen::AvlTree<MyStruct, NodeComparer, NodeFetcher>() };
		} == true);
	}
	SECTION("adding an object works") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree {};
		MyStruct v {};
		v.value = 10;
		REQUIRE(tree.add(v));
		REQUIRE(tree.size() == 1);
	}
	SECTION("adding the same object multiple times fails") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree {};
		MyStruct v {};
		v.value = 10;
		REQUIRE(tree.add(v));
		REQUIRE(tree.size() == 1);
		REQUIRE(!tree.add(v));
		REQUIRE(tree.size() == 1);
	}
	SECTION("adding an existing object to a different tree fails") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree1 {};
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree2 {};
		MyStruct v {};
		v.value = 10;
		REQUIRE(tree1.add(v));
		REQUIRE(tree1.size() == 1);
		REQUIRE(!tree2.add(v));
		REQUIRE(tree2.size() == 0);
	}
	SECTION("adding many objects") {
		constexpr size_t COUNT = 20000;
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree {};

		std::vector<std::shared_ptr<MyStruct>> structs {};
		structs.reserve(COUNT);
		for(size_t _ = 0; _ < COUNT; ++_) {
			const auto value = random();
			structs.push_back(std::make_shared<MyStruct>(MyStruct { (int)value }));
		}

		for(auto& s : structs) {
			REQUIRE(tree.add(*s));
		}
		REQUIRE(tree.size() == COUNT);
	}

	constexpr auto validate_iterator = [](auto it, std::vector<gen::AvlNode*> required_order) {
		std::vector<gen::AvlNode*> out {};
		for(auto* node : it) {
			out.push_back(node);
		}
		REQUIRE(out == required_order);
	};

	SECTION("adding multiple objects with the same value") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree {};
		MyStruct v1 { .value = 10 }, v2 { .value = 10 }, v3 { .value = 10 };

		REQUIRE(tree.add(v1));
		REQUIRE(tree.add(v2));
		REQUIRE(tree.add(v3));

		validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v1.node, &v2.node, &v3.node });
	}

	SECTION("iteration over the tree") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree {};
		MyStruct v40 { .value = 40 }, v20 { .value = 20 }, v10 { .value = 10 }, v30 { .value = 30 },
		        v50 { .value = 50 }, v60 { .value = 60 };
		REQUIRE(tree.add(v40));
		REQUIRE(tree.add(v20));
		REQUIRE(tree.add(v10));
		REQUIRE(tree.add(v30));
		REQUIRE(tree.add(v50));
		REQUIRE(tree.add(v60));

		SECTION("iteration order: preorder") {
			//  Preorder: root -> left -> right
			validate_iterator(tree.iterate<gen::TreeIteration::PreOrder>(),
			                  { &v40.node, &v20.node, &v10.node, &v30.node, &v50.node, &v60.node });
		}

		SECTION("iteration order: inorder") {
			//  Inorder: left -> root -> right
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(),
			                  { &v10.node, &v20.node, &v30.node, &v40.node, &v50.node, &v60.node });
		}

		SECTION("iteration order: postorder") {
			//  Postorder: left -> right -> root
			validate_iterator(tree.iterate<gen::TreeIteration::PostOrder>(),
			                  { &v10.node, &v30.node, &v20.node, &v60.node, &v50.node, &v40.node });
		}
	}

	SECTION("empty tree iteration") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> empty_tree {};
		validate_iterator(empty_tree.iterate<gen::TreeIteration::InOrder>(), {});
		validate_iterator(empty_tree.iterate<gen::TreeIteration::PreOrder>(), {});
		validate_iterator(empty_tree.iterate<gen::TreeIteration::PostOrder>(), {});
	}

	SECTION("single node tree iteration") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> single_node_tree {};
		MyStruct single { .value = 10 };
		REQUIRE(single_node_tree.add(single));

		validate_iterator(single_node_tree.iterate<gen::TreeIteration::InOrder>(), { &single.node });
		validate_iterator(single_node_tree.iterate<gen::TreeIteration::PreOrder>(), { &single.node });
		validate_iterator(single_node_tree.iterate<gen::TreeIteration::PostOrder>(), { &single.node });
	}

	SECTION("removal of objects from the tree") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree {};
		MyStruct v40 { .value = 40 }, v20 { .value = 20 }, v10 { .value = 10 }, v30 { .value = 30 },
		        v50 { .value = 50 }, v60 { .value = 60 };
		REQUIRE(tree.add(v40));
		REQUIRE(tree.add(v20));
		REQUIRE(tree.add(v10));
		REQUIRE(tree.add(v30));
		REQUIRE(tree.add(v50));
		REQUIRE(tree.add(v60));

		SECTION("all elements are present after adding") {
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(),
			                  { &v10.node, &v20.node, &v30.node, &v40.node, &v50.node, &v60.node });
		}

		SECTION("removing an element") {
			REQUIRE(tree.remove(v40));
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(),
			                  { &v10.node, &v20.node, &v30.node, &v50.node, &v60.node });

			REQUIRE(tree.remove(v60));
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(),
			                  { &v10.node, &v20.node, &v30.node, &v50.node });

			REQUIRE(tree.remove(v20));
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node, &v30.node, &v50.node });

			REQUIRE(tree.remove(v30));
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node, &v50.node });

			REQUIRE(tree.remove(v10));
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v50.node });

			REQUIRE(tree.remove(v50));
			validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), {});
		}
	}

	SECTION("detail: removal of objects from the tree") {
		gen::AvlTree<MyStruct, NodeComparer, NodeFetcher> tree {};
		MyStruct v40 { .value = 40 }, v20 { .value = 20 }, v10 { .value = 10 }, v30 { .value = 30 },
		        v50 { .value = 50 }, v60 { .value = 60 };

		SECTION("removing nodes with 0 children") {
			SECTION("removing root with no children") {
				REQUIRE(tree.add(v10));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node });
				REQUIRE(tree.remove(v10));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), {});

				REQUIRE(tree.add(v20));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v20.node });
				REQUIRE(tree.remove(v20));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), {});
			}

			SECTION("removing leaf") {
				REQUIRE(tree.add(v10));
				REQUIRE(tree.add(v20));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node, &v20.node });
				REQUIRE(tree.remove(v20));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node });
			}
		}

		SECTION("removing nodes with 1 child") {
			SECTION("removing root with 1 child") {
				REQUIRE(tree.add(v10));
				REQUIRE(tree.add(v20));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node, &v20.node });

				REQUIRE(tree.remove(v10));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v20.node });
			}

			SECTION("removing non-root with 1 child") {
				REQUIRE(tree.add(v40));
				REQUIRE(tree.add(v20));
				REQUIRE(tree.add(v10));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node, &v20.node, &v40.node });

				REQUIRE(tree.remove(v20));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v10.node, &v40.node });
			}
		}

		SECTION("removing nodes with 2 children") {
			SECTION("removing root with 2 children") {
				REQUIRE(tree.add(v40));
				REQUIRE(tree.add(v20));
				REQUIRE(tree.add(v50));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v20.node, &v40.node, &v50.node });

				REQUIRE(tree.remove(v40));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(), { &v20.node, &v50.node });
			}

			SECTION("removing non-root with 2 children") {
				REQUIRE(tree.add(v40));
				REQUIRE(tree.add(v20));
				REQUIRE(tree.add(v50));
				REQUIRE(tree.add(v10));
				REQUIRE(tree.add(v30));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(),
				                  { &v10.node, &v20.node, &v30.node, &v40.node, &v50.node });

				REQUIRE(tree.remove(v20));
				validate_iterator(tree.iterate<gen::TreeIteration::InOrder>(),
				                  { &v10.node, &v30.node, &v40.node, &v50.node });
			}
		}
	}
}