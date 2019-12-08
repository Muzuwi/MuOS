#include <Arch/i386/paging.hpp>
#include <Kernel/Debug/kdebugf.hpp>

#define PAGE_SIZE 4096

uint32_t page_table[1024][1024] __attribute__((__aligned__(4096)));
uint32_t page_dir[1024] __attribute__((__aligned__(4096)));

extern uint32_t _ukernel_start, _ukernel_end;
extern uint32_t _ukernel_virtual_offset;

dir_flags get_directory_flags(size_t index){
	// assert(index < 1024);
	if(index >= 1024){
		kerrorf("Tried reading flags from nonexistent page directory %i", index);
		return (dir_flags)0;
	} else {
		return (dir_flags)(page_dir[index] & 0x1FFF);
	}
}


page_flags get_page_flags(size_t dir, size_t page){
	if(dir >= 1024 || page >= 1024){
		kerrorf("Tried reading flags from nonexistent page directory/table %i:%i", dir, page);
		return (page_flags)0;
	} else {
		return (page_flags)(page_table[dir][page] & 0x1FFF);
	}
}


inline bool check_page_present(size_t dir, size_t page){
	return get_page_flags(dir,page) & PAGE_PRESENT;
}

inline bool check_dir_exists(size_t dir){
	return get_directory_flags(dir) & DIR_PRESENT;
}


uint32_t create_page(void* physical_address, page_flags flags){
	uint32_t page = 0;
	page |= ((uint32_t)physical_address & 0xFFFFF000);
	page |= flags;
	return page;
}

uint32_t create_directory(void* page_table_address, dir_flags flags){
	uint32_t dir = 0;
	dir |= (((uint32_t)page_table_address - (uint32_t)&_ukernel_virtual_offset) & 0xFFFFF000);
	dir |= flags;
	return dir;
}

/*
	Sets up a new page directory
*/
void new_directory(size_t dir){
	if(dir >= 1024) {
		kerrorf("[paging] Tried creating a new page directory with out of bounds index\n");
		return;
	} 
	kdebugf("[paging] New page directory %i\n", dir);
	page_dir[dir] = create_directory(&page_table[dir], (dir_flags)(DIR_PRESENT | DIR_RW));
}

/*
	Sets up a new page with default flags (only PRESENT)
*/
void new_page(size_t dir, size_t page, void* physical_address){
	if(dir >= 1024 || page >= 1024) {
		kerrorf("[paging] Tried creating a new page with out of bounds index\n");
		return;
	}
	// kdebugf("[paging] New page %i in directory %i\n", page, dir);
	page_table[dir][page] = create_page(physical_address, (page_flags)(PAGE_PRESENT));
}

/*
	Sets up a new page with specified args and flags
*/
void new_page(size_t dir, size_t page, void* physical_address, page_flags flags){
	if(dir >= 1024 || page >= 1024) {
		kerrorf("[paging] Tried creating a new page with out of bounds index\n");
		return;
	}
	// kdebugf("[paging] New page %i in directory %i\n", page, dir);
	page_table[dir][page] = create_page(physical_address, (page_flags)(PAGE_PRESENT | flags));
}


/*
	Creates a new page (and directory, if necessary) to map
	the passed physical address to virtual_address
*/
void Paging::allocate_page(void* physical_address, void* virtual_address, bool invalidate){
	size_t dir_index  = (uint32_t)virtual_address >> 22,
		   page_index = ((uint32_t)virtual_address >> 12) & 0x03FF; 

	//  Check if a directory exists
	if(!check_dir_exists(dir_index)){
		new_directory(dir_index);
	}

	//  Check if a page is already mapped there
	if(!check_page_present(dir_index, page_index)){
		new_page(dir_index, page_index, physical_address);
	} else {
		kerrorf("[paging] Tried allocating an already present page %i:%i\n", dir_index, page_index);
	}

	//  Flush TLB
	//  TODO: Is this even right?
	if(invalidate){
		asm volatile("invlpg (%0)\n"
			: 
			: ""((uint32_t)physical_address)
			: "memory");		
	}
}

/*
	Initializes the page directory and tables for the 
	first 4 MiB 
*/
void Paging::init_paging() {
	kdebugf("[paging] Initializing page structure\n");
	kdebugf("[paging] kernel start: %x, kernel end: %x\n", (uint32_t)&_ukernel_start, (uint32_t)&_ukernel_end);

	//  Mark the first 2 directories as not present,
	//	i.e. unmap the first 8MiB 
	for(size_t i = 0; i < 2; i++){
		page_dir[i] &= ~1;
	}

	//  Map all necessary pages to fit the entire kernel binary 
	uint32_t kernel_size = (uint32_t)(&_ukernel_end - &_ukernel_start);
	uint32_t kernel_dir_first = ((uint32_t)&_ukernel_start) / 0x400000;
	kdebugf("[paging] kernel size: %x, first directory: %i\n", kernel_size, kernel_dir_first);

	uint32_t current_vaddress = (uint32_t)&_ukernel_start;
	while(current_vaddress < (uint32_t)&_ukernel_end){
		allocate_page((void*)(current_vaddress - (uint32_t)&_ukernel_virtual_offset), (void*)(current_vaddress));
		current_vaddress += PAGE_SIZE;
	}

	kdebugf("[paging] last mapped kernel virtual address: %x\n", current_vaddress - PAGE_SIZE);
	uint32_t last_page = ((uint32_t)current_vaddress >> 12) & 0x03FF,
			 last_dir = (uint32_t)current_vaddress >> 22;

	//  FIXME: Should the stage0 mappings be removed afterwards?
	// kdebugf("[paging] erased stage0 mapping from %x", last_dir*0x400000 + last_page*4096);
	// while(page_table[last_dir][last_page] & 1){
	// 	page_table[last_dir][last_page] = 0;
	// 	if(last_page == 1023) {
	// 		last_dir++;
	// 		last_page = 0;
	// 		continue;
	// 	}
	// 	last_page++; 
	// }
	// kdebugf(" to %x\n", last_dir*0x400000 + last_page*4096);

	// auto page_dir_physical = ((uint32_t)&page_dir - (uint32_t)&_ukernel_virtual_offset);
	// asm volatile(
	// 	"mov %%eax, %0\n"
	// 	"mov cr3, %%eax\n"
	// 	"mov %%eax, cr0\n"
	// 	"or %%eax, 0x80000001\n"
	// 	"mov cr0, %%eax\t\n"
	// 	:
	// 	: ""(page_dir_physical) 
	// 	:
	// );
}

/*
	Checks whether a given virtual address is already mapped to some physical address
*/
bool Paging::is_present(void* virtual_address) {
	uint32_t dir = (uint32_t)virtual_address >> 22,
			 page = ((uint32_t)virtual_address >> 12) & 0x03FF;
	return check_dir_exists(dir) &&
		   check_page_present(dir, page);
}