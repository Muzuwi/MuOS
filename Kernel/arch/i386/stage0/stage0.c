
/*
	This function is responsible for setting up paging 
	for the kernel in the higher half
	This function is linked in at higher half, but
	caution must be taken with memory operations because of it.
*/

extern unsigned int _ukernel_virtual_offset;
extern unsigned int page_table[1024][1024];
extern unsigned int page_dir[1024];

//  Turn off optimizations in hopes of reducing the chance
//  of something trying to access higher half when not mapped
#pragma GCC optimize("O0")
void _stage0_entrypoint(){
	unsigned int* actual_address_dir = (unsigned int*)((unsigned int)&page_dir[0] - (unsigned int)&_ukernel_virtual_offset);	
	unsigned int* actual_address_table = (unsigned int*)((unsigned int)&page_table[0][0] - (unsigned int)&_ukernel_virtual_offset);

	actual_address_dir[0] |= 3;
	actual_address_dir[0] |= (unsigned int)(&actual_address_table[0]);

	actual_address_dir[1] |= 3;
	actual_address_dir[1] |= ((unsigned int)&actual_address_table[0] + 1*0x1000);

	//  Identity map the first 8MiB, hopefully the kernel doesn't grow beyond that for now..
	unsigned int* table_zero = (unsigned int*)(&actual_address_table[0]);
	unsigned int* table_one = (unsigned int*)((unsigned int)&actual_address_table[0] + 1*0x1000);
	int i = 0;
	while(i < 1024){
		table_zero[i] |= 3;
		table_zero[i] |= i*4096;
		i++;
	}

	i = 0;
	while(i < 1024){
		table_one[i] |= 3;
		table_one[i] |= 1024*4096 + i*4096;
		i++;
	}

	//  Bootstrap the first 8MiB of kernel in higher half, more accurate mappings will be done in higher half
	unsigned int dir_number = ((unsigned int)&_ukernel_virtual_offset / (4096*1024));
	actual_address_dir[dir_number] |= 3;
	actual_address_dir[dir_number] |= ((unsigned int)&actual_address_table[0] + dir_number*4096);
	actual_address_dir[dir_number+1] |= 3;
	actual_address_dir[dir_number+1] |= ((unsigned int)&actual_address_table[0] + (dir_number+1)*4096);

	i = 0;
	unsigned int* table_first = (unsigned int*)((unsigned int)&actual_address_table[0] + (dir_number)*0x1000);
	unsigned int* table_second = (unsigned int*)((unsigned int)&actual_address_table[0] + (dir_number+1)*0x1000);
	while(i < 1024){
		table_first[i] |= 3;
		table_first[i] |= 0 + i*4096;
		i++;
	}

	i = 0;
	while(i < 1024){
		table_second[i] |= 3;
		table_second[i] |= 0 + 1024*4096 + i*4096;
		i++;
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