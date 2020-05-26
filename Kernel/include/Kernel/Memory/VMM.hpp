#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Arch/i386/PageDirectory.hpp>
#include <Kernel/Symbols.hpp>

class VMapping;

class VMM final {
	friend class VMapping;

	VMM() {}
	static PageDirectory* s_kernel_directory;
	static void notify_create_VMapping(VMapping&);
	static void notify_free_VMapping(VMapping&);
	static PageTable& ensure_page_table(PageDirectoryEntry&);
public:
	static PageDirectory* get_directory();
	static VMM& get();
	static void init();

	void map(uintptr_t*, uintptr_t*);
	void unmap(uintptr_t*);

	static void* allocate_user_stack(size_t stack_size);
};
