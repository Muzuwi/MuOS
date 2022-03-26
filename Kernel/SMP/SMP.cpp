#include <APIC/APIC.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/IDT.hpp>
#include <LibGeneric/Vector.hpp>
#include <Memory/Ptr.hpp>
#include <Memory/VMM.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <SMP/SMP.hpp>

ControlBlock SMP::s_bootstrap_ctb { static_cast<uint8>(-1) };

gen::Vector<ControlBlock*> SMP::s_attached_aps {};

void SMP::attach_boot_ap() {
	const uint8 bsp = APIC::ap_bootstrap_id();
	s_bootstrap_ctb.m_apic_id = bsp;
	attach_ap(&s_bootstrap_ctb);
}

void SMP::init_boot_ap_gdt(void* ptr) {
	s_bootstrap_ctb.m_gdt = static_cast<GDT*>(ptr);
}

void SMP::reload_boot_ctb() {
	CPU::set_gs_base(&s_bootstrap_ctb);
	CPU::set_kernel_gs_base(&s_bootstrap_ctb);
}

ControlBlock& SMP::ctb() {
	auto read_ctlblock_from_gs = []() -> void* {
		uint64 data;
		asm volatile("mov %0, %%gs:0\n" : "=r"(data) :);
		return (void*)data;
	};

	auto* ptr = read_ctlblock_from_gs();
	kassert(ptr);
	return *reinterpret_cast<ControlBlock*>(ptr);
}

void SMP::attach_ap(ControlBlock* ctb) {
	ctb->m_vid = s_attached_aps.size();
	klogf_static("[SMP] Attaching AP(APICID={x}) with VID={}\n", ctb->apic_id(), ctb->vid());
	s_attached_aps.push_back(ctb);
}

/*
 *  The entrypoint used by all starting APs.
 *  Prepares the current AP for running the scheduler loop and enters into the idle task for the first time.
 */
void SMP::ap_entrypoint(ControlBlock* ctb, Thread* idle_task, PhysAddr code_page, PhysAddr data_page) {
	CPU::set_gs_base(ctb);
	CPU::set_kernel_gs_base(ctb);
	IDT::init_ap();
	GDT::init_ap_gdt(ctb);
	CPU::initialize_features();
	CPU::set_gs_base(ctb);
	CPU::set_kernel_gs_base(ctb);
	idle_task->parent()->vmm().addrunmap(code_page.get());
	idle_task->parent()->vmm().addrunmap(data_page.get());

	SMP::attach_ap(ctb);
	this_cpu().scheduler().m_ap_idle = idle_task;

	klogf_static("[AP(VID={})] AP running kernel code. APIC ID={x}\n", this_cpu().vid(), this_cpu().apic_id());

	uint8 _dummy_val[sizeof(Thread)] = {};
	//  Huge hack - use dummy buffer on the stack when switching for the first time,
	//  as we don't care about saving garbage data
	CPU::switch_to(reinterpret_cast<Thread*>(&_dummy_val), idle_task);
}
