#include <Kernel/Memory/VirtualMemManager.hpp>
#include <Arch/i386/MemManager.hpp>
#include <Arch/i386/paging.hpp>
#include <Kernel/Debug/kdebugf.hpp>

//  Temporarily store the virt address given to kmalloc
static mem_range_t kmalloc_temp;

extern uint32_t _ukernel_start, _ukernel_end, _ukernel_virtual_start, _ukernel_virtual_offset;

VirtualMemManager::VirtualMemManager() {

}

VirtualMemManager& VirtualMemManager::get(){
	static VirtualMemManager vmemmanager;
	return vmemmanager;
}
