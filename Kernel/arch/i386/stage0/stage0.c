/*
	This function is responsible for setting up paging 
	for the kernel in the higher half
	This function is linked in at higher half, but
	caution must be taken with memory operations because of it.
*/

#define PAGE_SIZE 4096
#define MiB 0x100000

extern unsigned int _ukernel_virtual_offset;
extern unsigned int page_table[1024][1024];
extern unsigned int page_dir[1024];
extern unsigned int _ukernel_start, _ukernel_end;

#define kernel_size ((unsigned int)&_ukernel_end-(unsigned int)&_ukernel_start)
#define kernel_end_physical ((unsigned int)&_ukernel_end - (unsigned int)&_ukernel_virtual_offset)
#define kernel_start_physical ((unsigned int)&_ukernel_start - (unsigned int)&_ukernel_virtual_offset)

void _stage0_entrypoint(){
	/*
		Physical memory locations of the page directory and table
	*/	
	unsigned int* actual_address_dir = (unsigned int*)((unsigned int)&page_dir[0] - (unsigned int)&_ukernel_virtual_offset);	
	unsigned int* actual_address_table = (unsigned int*)((unsigned int)&page_table[0][0] - (unsigned int)&_ukernel_virtual_offset);

	/*
		Identity map 0-0x800000
		We need only the first few megs of so
		of kernel memory to jump to higher half
	*/
	for(int i = 0; i < 2; i++) {
		unsigned int* table_addr = (unsigned int*)((unsigned int)&actual_address_table[0] + i*0x1000);
		actual_address_dir[i] |= 3;
		actual_address_dir[i] |= (unsigned int)(table_addr);
		for(int table = 0; table < 1024; table++) {
			table_addr[table] |= 3;
			table_addr[table] |= i*4*MiB + table*PAGE_SIZE;
		}
	}

	/*
		Map the entire kernel binary
	*/
	unsigned int dir_first = (unsigned int)&_ukernel_start / (4*MiB);
	unsigned int dir_count = ((unsigned int)&_ukernel_end / (4*MiB)) - dir_first; 
	unsigned int dir_counter = dir_first;
	short finished = 0;
	while(dir_counter <= dir_first+dir_count && finished == 0) {
		unsigned int* table_addr = (unsigned int*)((unsigned int)&actual_address_table[0] + dir_counter*0x1000);
		actual_address_dir[dir_counter] |= 3;
		actual_address_dir[dir_counter] |= (unsigned int)(table_addr);
		for(int table = 0; table < 1024; table++) {
			unsigned int* addr = (unsigned int*)((dir_counter-dir_first)*4*MiB + table*PAGE_SIZE);
			if((unsigned int)addr >= kernel_end_physical) {
				finished = 1;
				break;
			}
			table_addr[table] |= 3;
			table_addr[table] |= (unsigned int)addr; 
		}

		dir_counter = dir_counter + 1;
	}

	//  Update cr3
	asm volatile(
		"mov %%eax, %0\n"
		"mov cr3, %%eax\n"
		"mov %%eax, cr0\n"
		"or %%eax, 0x80000001\n"
		"mov cr0, %%eax\t\n"
		:
		: ""((unsigned int)&actual_address_dir[0]) 
		:
	);

}