#pragma once

#include <LibGeneric/Vector.hpp>
#include <SMP/ControlBlock.hpp>

class PhysAddr;

class SMP {
	static ControlBlock s_bootstrap_ctb;
	static gen::Vector<ControlBlock*> s_attached_aps;

	static void ap_entrypoint(ControlBlock*, Thread*, PhysAddr, PhysAddr);
public:
	static void attach_boot_ap();
	static void attach_ap(ControlBlock*);

	static void init_boot_ap_gdt(void*);
	static void reload_boot_ctb();

	static ControlBlock& ctb();

	//  FIXME: Looks nasty
	static constexpr gen::Vector<ControlBlock*> const& attached_aps() { return s_attached_aps; }
};

static inline ControlBlock& this_cpu() {
	return SMP::ctb();
}

static inline Thread* this_thread() {
	return this_cpu().current_thread();
}
