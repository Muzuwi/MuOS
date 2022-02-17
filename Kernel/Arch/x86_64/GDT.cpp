#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Debug/klogf.hpp>
#include <SMP/SMP.hpp>

static GDT s_base_ap_gdt {};

void GDT::init_base_ap_gdt() {
	CPU::lgdt(&s_base_ap_gdt.m_descriptor);
	CPU::ltr(GDT::tss_sel);
	SMP::init_boot_ap_gdt(&s_base_ap_gdt);
	klogf_static("[GDT] GDT/TSS setup complete\n");
}

void GDT::init_ap_gdt(ControlBlock* ctb) {
	auto* maybe_gdt = KHeap::instance().chunk_alloc(sizeof(GDT));
	kassert(maybe_gdt);

	auto* gdt = new(maybe_gdt) GDT();
	CPU::lgdt(&gdt->m_descriptor);
	CPU::ltr(GDT::tss_sel);
	ctb->set_gdt(gdt);
}
