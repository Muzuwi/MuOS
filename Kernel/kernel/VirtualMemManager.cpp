#include <kernel/VirtualMemManager.hpp>
#include <arch/i386/MemManager.hpp>
#include <arch/i386/paging.hpp>
#include <kernel/kdebugf.hpp>

//  Temporarily store the virt address given to kmalloc
static mem_range_t kmalloc_temp;

extern uint32_t _ukernel_start, _ukernel_end, _ukernel_virtual_start, _ukernel_virtual_offset;

VirtualMemManager::VirtualMemManager() {

}

VirtualMemManager& VirtualMemManager::get(){
	static VirtualMemManager vmemmanager;
	return vmemmanager;
}

/*
	
*/
mem_range_t VirtualMemManager::bootstrap(uint64_t size) {
	auto phys_range = MemManager::get().allocate_kernel_range(size, 4096);
	if(size & 0xFFF){
		kdebugf("[VMM] bootstrap: size not page aligned!\n");
	}
	kdebugf("[VMM] bootstrap: got physical range %x-%x\n", (uint32_t)phys_range.m_start, (uint32_t)phys_range.m_end);
	//  Allocate kmalloc pool starting at next aligned page after the kernel
	uint32_t aligned = (((uint32_t)&_ukernel_end) & ~0xFFF) + 0x1000;
	kdebugf("[VMM] bootstrap: allocating %x-%x for kmalloc\n", aligned, aligned+size);

	uint32_t bytes = 0;
	while(bytes < size){
		Paging::allocate_page((void*)((uint32_t)phys_range.m_start+bytes),
							  (void*)(aligned + bytes));
		bytes += 4096;
	}

	kmalloc_temp = mem_range_t(aligned, aligned+size);

	return kmalloc_temp;
}