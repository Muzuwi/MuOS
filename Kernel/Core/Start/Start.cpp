#include "Start.hpp"
#include <Arch/MP.hpp>
#include <Arch/Platform.hpp>
#include <Arch/x86_64/PIT.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Mem/Layout.hpp>
#include <Core/MP/MP.hpp>
#include <Drivers/IDE/IDE.hpp>
#include <Memory/VMM.hpp>
#include <Scheduler/Scheduler.hpp>
#include "Core/Error/Error.hpp"
#include "Core/Log/Logger.hpp"
#ifdef KERNEL_HACKS_VESADEMO
#	include "Daemons/DemoVESA/DemoVESA.hpp"
#endif
#include "Daemons/Kbd/Kbd.hpp"
#include "Daemons/SysDbg/SysDbg.hpp"
#include "Daemons/Testd/Testd.hpp"
#include "LibFormat/Formatters/Pointer.hpp"
#include "LibGeneric/String.hpp"
#include "Process/Process.hpp"

CREATE_LOGGER("core::start", core::log::LogLevel::Debug);

/*	Dumps the physical memory layout and it's current kernel reservations.
 */
static void dump_core_mem_layout() {
	::log.info("Physical memory layout:");
	core::mem::for_each_region([](core::mem::Region region) {
		::log.info("|- {x} - {x} ({})", Format::ptr(region.start), Format::ptr((uint8*)region.start + region.len),
		           core::mem::type_to_str(region.type));
	});
}

/** Start the kernel and boot into userland
 *
 * 	This is the main entrypoint for the non-platform specific kernel.
 */
[[noreturn, maybe_unused]] void core::start::start() {
	//  Meant to handle early debugging stuff, things that need to be
	//  init'd as soon as possible
	if(const auto err = arch::platform_early_init(); err != core::Error::Ok) {
		//  FIXME: Better error handling, break into debugger?
		::log.fatal("Platform early init failed with error code: {}", err);
		ENSURE_NOT_REACHED();
	}
	::log.info("Platform early init done");

	auto* env = core::mp::create_environment();
	arch::mp::environment_set(env);
	::log.info("Kernel starting on node={} with environment={x}", env->node_id, Format::ptr(env));

	dump_core_mem_layout();
	//  Initialize required kernel subsystems
	VMM::initialize_kernel_vm();

	//  Handles further platform initialization tasks
	if(const auto err = arch::platform_init(); err != core::Error::Ok) {
		//  FIXME: Better error handling, break into debugger?
		::log.fatal("Platform init failed with error code: {}", err);
		ENSURE_NOT_REACHED();
	}
	::log.info("Platform init done");
	::log.info("Kernel init done, time passed: {}ms", PIT::milliseconds());

	//  Prepare the next stage of initialization
	//  Late init is called within a multitasking environment, so it's
	//  safe to use more advanced locking primitives
	auto thread =
	        Process::create_with_main_thread(gen::String { "late_init" }, Process::kerneld(), core::start::late_init);
	core::mp::bootstrap_this_node(nullptr, thread.get());
}

/**
 * Perform late initialization of device drivers.
 * Late init is called within a proper multitasking environment within
 * a separate late_init kernel process.
 */
[[noreturn, maybe_unused]] void core::start::late_init() {
	//  Initialize drivers
	(void)driver::ide::init();

	//  Spawn the kernel serial debugger
	auto serial_dbg =
	        Process::create_with_main_thread(gen::String { "sys_dbg" }, Process::kerneld(), SysDbg::sysdbg_thread);
	this_cpu()->scheduler->run_here(serial_dbg.get());

#ifdef KERNEL_HACKS_VESADEMO
	//  Spawn the VESA demo thread
	auto vesa_demo =
	        Process::create_with_main_thread(gen::String { "vesademo" }, Process::kerneld(), vesademo::demo_thread);
	this_cpu()->scheduler->run_here(vesa_demo.get());
#endif

	//  Spawn a demo thread that reads from the keyboard
	auto kbd = Process::create_with_main_thread(gen::String { "debug_keyboard" }, Process::kerneld(), Kbd::kbd_thread);
	kbd->sched_ctx().priority = 0;
	this_cpu()->scheduler->run_here(kbd.get());

	//  Spawn a demo userland "init" thread
	//  This is just a quick test to see if jumping to ring 3
	//  works properly, doesn't actually do any meaningful work.
	auto userland_init = Process::init();
	ENSURE(userland_init->vmm().clone_address_space_from(Process::kerneld()->vmm().paging_handle()));
	auto init_thread = Thread::create_in_process(userland_init, Testd::userland_test_thread);
	init_thread->sched_ctx().priority = 10;
	this_cpu()->scheduler->run_here(init_thread.get());

	::log.info("Late init completed");
	this_cpu()->scheduler->sleep();
	//  We should never be awoken, but put a safe guard anyway
	while(true)
		;
}
