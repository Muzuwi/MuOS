#include <stdint.h>
#include <stddef.h>
#include <Kernel/Symbols.hpp>


#define GET_DIR(a) ((uint32_t)a >> 22)

//  Page Table used for bootstrapping higher-half, otherwise we would
//  triple fault the moment paging is enabled
uint32_t zerodir_trampoline[1024] __attribute__((aligned(4096)));

/*
 *	Initializes paging for the kernel in the higher-half
 */
extern "C" void _paging_bootstrap() {
	//  Kernel page directory
	extern uint32_t s_kernel_directory_table;

	//  Physical addresses of the page directory and page memory of the kernel
	uint32_t *phys_dir = TO_PHYS(&s_kernel_directory_table);
	uint32_t *phys_trampoline = TO_PHYS(zerodir_trampoline);
	uint32_t *phys_kernel_pages = TO_PHYS(&_ukernel_pages_start);

	//  First PD of kernel virtual address space
	uint32_t first_dir = GET_DIR((uint32_t)(&_ukernel_virtual_offset));

	/*
	 *	Identity map 0 - 4MB
	 */
	phys_dir[0] = ((uint32_t)phys_trampoline) | 3;
	for(size_t i = 0; i < 1024; i++) {
		phys_trampoline[i] = i*4096 | 3;
	}

	/*
	 *	Map the entire kernel executable
	 *  TODO:  Permissions, but those can also be done in the kernel itself
	 */
	bool cont = true;
	for(size_t i = first_dir; (i < 1024) && cont; i++) {
		uint32_t *table_address = (phys_kernel_pages) + (i-first_dir)*4096;
		phys_dir[i] = (uint32_t)table_address | 3;

		for(size_t j = 0; (j < 1024) && cont; j++) {
			uint32_t where = (i-first_dir)*0x400000 + j*4096;
			if(where >= (uint32_t)&_ukernel_end - (uint32_t)&_ukernel_virtual_offset) {
				cont = false;
				break;
			}
			table_address[j] = where | 3;
		}
	}

	asm volatile(
	//  Update CR3 and enable paging bit
	    "mov %%eax, %0\n"
	    "mov cr3, %%eax\n"
	    "mov %%eax, cr0\n"
	    "or %%eax, 0x80000001\n"
	    "mov cr0, %%eax\n"
	 //  Fix esp
	    "mov %%eax, %%esp\n"
	    "add %%eax, %0\n"
	    "mov %%esp, %%eax\n"
	 //  Jump to higher half
	    "mov %%eax, %1\n"
	    "push %%eax\n"
	    "ret\n"     //  Cannot trust normal function returns at this point, as stack protector
	                //  initializes the canary to an undefined value because virtual mappings
	                //  have not been initialized yet. Instead the bootstrap now jumps to the
	                //  kernel higher entrypoint by itself.
	    :: ""((uint32_t)phys_dir), ""((uint32_t)&_ukernel_higher_entrypoint)
	    : "eax"
	);
}
