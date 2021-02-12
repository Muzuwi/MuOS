#pragma once
#include <Kernel/SystemTypes.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <Arch/i386/Paging.hpp>

class VMapping;
enum class MappingPrivilege;

class VMM final {
	friend class VMapping;

//	VMM() {}
//	static PageDirectory* s_kernel_directory;
//	static void notify_create_VMapping(gen::SharedPtr<VMapping>, MappingPrivilege);
//	static void notify_free_VMapping(VMapping&);
//	static PageTable& ensure_page_table(PageDirectoryEntry&);

	static PDPTE& ensure_pdpte(void*);
	static PDE& ensure_pde(void*);
	static PTE& ensure_pte(void*);
	static void map_kernel_executable();
	static void map_physical_identity();
public:
//	static PageDirectory* get_directory();
//	static VMM& get();
//	static void init();
//
//	static void* allocate_user_stack(size_t stack_size);
//	static void* allocate_interrupt_stack();
//	static void* allocate_kerneltask_stack();
	static void init();
};
