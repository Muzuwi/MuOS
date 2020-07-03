#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Arch/i386/PageDirectory.hpp>
#include <Kernel/Symbols.hpp>

class VMapping;
enum class MappingPrivilege;

class VMM final {
	friend class VMapping;

	VMM() {}
	static PageDirectory* s_kernel_directory;
	static void notify_create_VMapping(VMapping&, MappingPrivilege);
	static void notify_free_VMapping(VMapping&);
	static PageTable& ensure_page_table(PageDirectoryEntry&);
public:
	static PageDirectory* get_directory();
	static VMM& get();
	static void init();

	static void* allocate_user_stack(size_t stack_size);
	static void* allocate_interrupt_stack();
	static void* allocate_kerneltask_stack();
};
