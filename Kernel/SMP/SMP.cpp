#include <Arch/i386/CPU.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/Vector.hpp>
#include <Kernel/APIC/APIC.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/SMP/SMP.hpp>

//  FIXME: Don't store these, but initialize the APs with the control blocks as we initialize them
static gen::Vector<ControlBlock*> s_control_blocks {};
static ControlBlock s_bootstrap_ctl{static_cast<uint8>(-1)};


void SMP::bootstrap_ctb() {
	CPU::set_gs_base(&s_bootstrap_ctl);
	CPU::set_kernel_gs_base(&s_bootstrap_ctl);
}

void SMP::init_control_blocks() {
	const auto& aps = APIC::ap_list();
	const uint8 bsp = APIC::ap_bootstrap_id();
	s_bootstrap_ctl.set_ap(bsp);
	return;

	s_control_blocks.reserve(aps.size());
	for(uint8 ap : aps) {
		auto* control_block = new (KHeap::allocate(sizeof(ControlBlock))) ControlBlock(ap);
		s_control_blocks.push_back(control_block);

		//  FIXME: Initialize rest of the AP's control blocks, when SMP is actually ready to use
		if(ap == bsp) {
			CPU::set_gs_base(control_block);
		}
	}
}


void* read_ctlblock_from_gs() {
	uint64 data;
	asm volatile(
			"mov %0, %%gs:0\n"
			:"=r"(data)
			:
			);
	return (void*)data;
}

ControlBlock& SMP::ctb() {
	auto* ptr = read_ctlblock_from_gs();
	kassert(ptr);
	return *reinterpret_cast<ControlBlock*>(ptr);
}
