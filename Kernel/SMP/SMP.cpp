#include <APIC/APIC.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/IDT.hpp>
#include <LibGeneric/Vector.hpp>
#include <Memory/Ptr.hpp>
#include <Memory/VMM.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <SMP/SMP.hpp>

static ControlBlock s_bootstrap_ctl { static_cast<uint8>(-1) };

void SMP::bootstrap_ctb() {
	CPU::set_gs_base(&s_bootstrap_ctl);
	CPU::set_kernel_gs_base(&s_bootstrap_ctl);
}

void SMP::init_control_blocks() {
	const auto& aps = APIC::ap_list();
	const uint8 bsp = APIC::ap_bootstrap_id();
	s_bootstrap_ctl.set_ap(bsp);
}

void SMP::init_boot_ap_gdt(void* ptr) {
	s_bootstrap_ctl.set_gdt(static_cast<GDT*>(ptr));
}

void* read_ctlblock_from_gs() {
	uint64 data;
	asm volatile("mov %0, %%gs:0\n" : "=r"(data) :);
	return (void*)data;
}

ControlBlock& SMP::ctb() {
	auto* ptr = read_ctlblock_from_gs();
	kassert(ptr);
	return *reinterpret_cast<ControlBlock*>(ptr);
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

	const uint8 local_id = APIC::lapic_read(LAPICReg::APICID) >> 24u;
	klogf_static("Hello world from AP with APICID={x}, ctb ID={x}\n", local_id, SMP::ctb().current_ap());
	klogf_static("CTB{{{}}}, Idle{{{}}}, code{{{}}}, data{{{}}}\n", Format::ptr(ctb), Format::ptr(idle_task),
	             code_page.get(), data_page.get());

	SMP::ctb().scheduler().m_ap_idle = idle_task;

	//	while(true) {
	//		asm volatile("cli\nhlt\n");
	//	}

	uint8 _dummy_val[sizeof(Thread)] = {};
	//  Huge hack - use dummy buffer on the stack when switching for the first time,
	//  as we don't care about saving garbage data
	CPU::switch_to(reinterpret_cast<Thread*>(&_dummy_val), idle_task);
}
