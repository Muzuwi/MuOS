#include <APIC/APIC.hpp>
#include <Daemons/BootAP/BootAP.hpp>
#include <Debug/kassert.hpp>
#include <Memory/PMM.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <SMP/APBootstrap.hpp>
#include <SMP/SMP.hpp>
#include <Scheduler/Scheduler.hpp>
#include <SystemTypes.hpp>
#include <Kernel/ksleep.hpp>
#include <Debug/klogf.hpp>

extern uint8 ap_bootstrap_start;
extern uint8 ap_bootstrap_end;

void BootAP::boot_ap_thread() {
	klogf("[BootAP({})]: Booting up APs\n", Thread::current()->tid());
	auto* thread = SMP::ctb().current_thread();
	auto const& process = thread->parent();

	auto maybe_code_page = PMM::instance().allocate_lowmem();
	if(!maybe_code_page.has_value()) {
		klogf("[BootAP({})]: Lowmem alloc for code failed\n", thread->tid());
		Thread::current()->set_state(TaskState::Blocking);
		SMP::ctb().scheduler().schedule();
		ASSERT_NOT_REACHED();
	}
	auto maybe_data_page = PMM::instance().allocate_lowmem();
	if(!maybe_data_page.has_value()) {
		klogf("[BootAP({})]: Lowmem alloc for data failed\n", thread->tid());
		Thread::current()->set_state(TaskState::Blocking);
		SMP::ctb().scheduler().schedule();
		ASSERT_NOT_REACHED();
	}

	auto code_page = maybe_code_page.unwrap();
	auto data_page = maybe_data_page.unwrap();
	process->vmm().addrmap(code_page.get(), code_page,
	                       static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL));
	process->vmm().addrmap(data_page.get(), data_page,
	                       static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL));
	klogf("[BootAP({})]: Code at {}, data at {}\n", thread->tid(), code_page.get(), data_page.get());

	const uint8 ipi_vector = (uintptr_t)code_page.get() / 0x1000;
	kassert(ipi_vector < 0xA0 || ipi_vector > 0xBF);


	for(auto& ap_id : APIC::ap_list()) {
		if(ap_id == APIC::ap_bootstrap_id()) {
			continue;
		}
		klogf("[BootAP({})]: Booting up AP {}\n", thread->tid(), ap_id);

		memcpy(code_page.get_mapped(),
		       static_cast<void const*>(&ap_bootstrap_start),
		       &ap_bootstrap_end - &ap_bootstrap_start);
		//  Pass the data page address
		*(code_page.as<uint16>() + 1) = (uintptr_t)data_page.get();

		auto idle_task = SMP::ctb().scheduler().create_idle_task();
		APBoostrap bootstrap_struct {};
		bootstrap_struct.real_gdtr_offset = (uintptr_t)data_page.get() + offsetof(APBoostrap, real_mode_gdt);
		bootstrap_struct.compat_gdtr_offset = (uintptr_t)data_page.get() + offsetof(APBoostrap, compat_mode_gdt);
		bootstrap_struct.long_gdtr_offset = (uintptr_t)data_page.get() + offsetof(APBoostrap, long_mode_gdt);
		bootstrap_struct.cr3 = (uintptr_t)idle_task->parent()->vmm().pml4().get();

		//  Copy over the boot struct
		memcpy(data_page.get_mapped(),
		       &bootstrap_struct,
		       sizeof(APBoostrap));

		//  Try booting up the AP
		const uint32 id_msg = (uint32)ap_id << 24u;
		const uint32 lo_init = 0xC500;          //  Init IPI
		const uint32 lo_deassert_init = 0x8500; //  De-assert init IPI

		thread->preempt_disable();
		APIC::lapic_write(LAPICReg::ESR, 0x0);
		APIC::lapic_write(LAPICReg::ICRHi, id_msg);
		APIC::lapic_write(LAPICReg::ICRLow, lo_init);
		while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))  //  Wait for delivery
			;
		APIC::lapic_write(LAPICReg::ICRLow, lo_deassert_init);
		while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))  //  Wait for delivery
			;

		ksleep(10);

		for(unsigned i = 0; i < 2; ++i) {
			APIC::lapic_write(LAPICReg::ESR, 0x0);
			APIC::lapic_write(LAPICReg::ICRHi, id_msg);
			APIC::lapic_write(LAPICReg::ICRLow, 0x600 | ipi_vector);
			ksleep(1);
			while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))  //  Wait for delivery
				;
		}
		thread->preempt_enable();

		while(*data_page.as<uint64>() == 0);
		klogf("[BootAP({})]: AP {} started - waiting for long mode\n", thread->tid(), ap_id);
		while(*data_page.as<uint64>() != 2);
		klogf("[BootAP({})]: AP {} initialized\n", thread->tid(), ap_id);
//		break;
	}

	klogf("[BootAP({})]: Initialization done\n", thread->tid());

	process->vmm().addrunmap(code_page.get());
	process->vmm().addrunmap(data_page.get());

	PMM::instance().free_lowmem(code_page);
	PMM::instance().free_lowmem(data_page);

	Thread::current()->set_state(TaskState::Blocking);
	SMP::ctb().scheduler().schedule();
	ASSERT_NOT_REACHED();
}
