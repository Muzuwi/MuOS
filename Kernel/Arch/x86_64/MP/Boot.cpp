#include "Boot.hpp"
#include <Arch/x86_64/APIC.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/MP/CpuBootstrapPage.hpp>
#include <Core/Log/Logger.hpp>
#include <Kernel/ksleep.hpp>
#include <Memory/PMM.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include <SystemTypes.hpp>
#include "Arch/x86_64/IDT.hpp"
#include "Core/MP/MP.hpp"
#include "ExecutionEnvironment.hpp"
#include "Memory/Ptr.hpp"

extern uint8 ap_bootstrap_start;
extern uint8 ap_bootstrap_end;

CREATE_LOGGER("x86_64::ap_boot", core::log::LogLevel::Debug);

/**	Boot all APs available on the machine and walk them through to kernel mode
 */
void arch::mp::boot_aps() {
	log.info("Bringing up other APs");

	auto maybe_code_page = PMM::instance().allocate_lowmem();
	if(!maybe_code_page.has_value()) {
		log.error("AP bringup error - code page lowmem alloc failed");
		return;
	}
	auto maybe_data_page = PMM::instance().allocate_lowmem();
	if(!maybe_data_page.has_value()) {
		log.error("AP bringup error - data page lowmem alloc failed");
		return;
	}

	auto code_page = maybe_code_page.unwrap();
	auto data_page = maybe_data_page.unwrap();
	log.debug("Code={}, Data={}", code_page.get(), data_page.get());

	const uint8 ipi_vector = (uintptr_t)code_page.get() / 0x1000;
	if(!(ipi_vector < 0xA0 || ipi_vector > 0xBF)) {
		log.error("AP bringup error - Allocated page lands on unsupported IPI vector");
		return;
	}

	for(auto& ap_id : APIC::ap_list()) {
		if(ap_id == APIC::ap_bootstrap_id()) {
			continue;
		}
		log.info("Bringing up AP {x}...", ap_id);

		memcpy(code_page.get_mapped(), static_cast<void const*>(&ap_bootstrap_start),
		       &ap_bootstrap_end - &ap_bootstrap_start);
		//  Pass the data page address
		*(code_page.template as<uint16 volatile>() + 1) = (uintptr_t)data_page.get();

		auto idle_task = Scheduler::create_idle_task(ap_id);

		CpuBootstrapPage bootstrap_struct {};
		bootstrap_struct.real_gdtr_offset = (uintptr_t)data_page.get() + offsetof(CpuBootstrapPage, real_mode_gdt);
		bootstrap_struct.compat_gdtr_offset = (uintptr_t)data_page.get() + offsetof(CpuBootstrapPage, compat_mode_gdt);
		bootstrap_struct.long_gdtr_offset = (uintptr_t)data_page.get() + offsetof(CpuBootstrapPage, long_mode_gdt);
		bootstrap_struct.cr3 = (uintptr_t)idle_task->parent()->vmm().pml4().get();
		bootstrap_struct.ap_environment = arch::mp::create_environment();
		bootstrap_struct.ap_environment->apic_id = ap_id;
		bootstrap_struct.idle_task = idle_task;
		bootstrap_struct.rsp = idle_task->irq_task_frame();
		bootstrap_struct.code_page = code_page.get();
		bootstrap_struct.data_page = data_page.get();

		auto idled = idle_task->parent();
		idled->vmm().addrmap(code_page.get(), code_page,
		                     static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL));
		idled->vmm().addrmap(data_page.get(), data_page,
		                     static_cast<VMappingFlags>(VM_READ | VM_WRITE | VM_EXEC | VM_KERNEL));

		//  Copy over the boot struct
		memcpy(data_page.get_mapped(), &bootstrap_struct, sizeof(CpuBootstrapPage));

		//  Try booting up the AP
		const uint32 id_msg = (uint32)ap_id << 24u;
		const uint32 lo_init = 0xC500;         //  Init IPI
		const uint32 lo_deassert_init = 0x8500;//  De-assert init IPI

		APIC::lapic_write(LAPICReg::ESR, 0x0);
		APIC::lapic_write(LAPICReg::ICRHi, id_msg);
		APIC::lapic_write(LAPICReg::ICRLow, lo_init);
		while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))//  Wait for delivery
			;
		APIC::lapic_write(LAPICReg::ICRLow, lo_deassert_init);
		while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))//  Wait for delivery
			;

		ksleep(10);

		for(unsigned i = 0; i < 2; ++i) {
			APIC::lapic_write(LAPICReg::ESR, 0x0);
			APIC::lapic_write(LAPICReg::ICRHi, id_msg);
			APIC::lapic_write(LAPICReg::ICRLow, 0x600 | ipi_vector);
			ksleep(1);
			while(APIC::lapic_read(LAPICReg::ICRLow) & (1u << 12u))//  Wait for delivery
				;
		}

		while(*data_page.template as<uint64 volatile>() < 1)
			;
		log.debug("AP {x} started - waiting for long mode", ap_id);
		while(*data_page.template as<uint64 volatile>() < 2)
			;
		log.info("AP {x}: up", ap_id);
		//  The AP is responsible for unmapping the lowmem pages - after entering long mode,
		//  they should not attempt to access the bootstrap pages
	}

	log.info("AP bringup completed");
	PMM::instance().free_lowmem(code_page);
	PMM::instance().free_lowmem(data_page);
}

/*
 *  The entrypoint used by all starting APs.
 *  Prepares the current AP for running the scheduler loop and enters into the idle task for the first time.
 */
extern "C" void _ap_entrypoint(arch::mp::ExecutionEnvironment* env, Thread* idle_task, PhysAddr code_page,
                               PhysAddr data_page) {
	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);

	this_execution_environment()->gdt.load();
	IDT::init_ap();
	CPU::initialize_features();

	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);

	//  Clean up bootstrap pages
	idle_task->parent()->vmm().addrunmap(code_page.get());
	idle_task->parent()->vmm().addrunmap(data_page.get());

	log.debug("APIC node {x} up", env->apic_id);
	core::mp::bootstrap_this_node(idle_task);
}
