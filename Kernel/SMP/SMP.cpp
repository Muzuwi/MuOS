#include <Arch/x86_64/CPU.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/Vector.hpp>
#include <APIC/APIC.hpp>
#include <Memory/KHeap.hpp>
#include <SMP/SMP.hpp>

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
