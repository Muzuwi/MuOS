#include "Start.hpp"
#include <Arch/Interface.hpp>
#include <Arch/x86_64/PIT.hpp>
#include <Debug/kpanic.hpp>
#include <Drivers/IDE/IDE.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>
#include <SMP/SMP.hpp>
#include "Debug/klogf.hpp"
#include "LibGeneric/String.hpp"
#include "Process/Process.hpp"

/** Start the kernel and boot into userland
 *
 * 	This is the main entrypoint for the non-platform specific kernel.
 */
[[noreturn, maybe_unused]] void core::start::start() {
	//  Meant to handle early debugging stuff, things that need to be
	//  init'd as soon as possible
	if(const auto err = arch::platform_early_init(); err != core::Error::Ok) {
		//  FIXME: Better error handling, break into debugger?
		kerrorf("[uKernel] Platform early init failed with error code: {}\n", static_cast<size_t>(err));
		kpanic();
	}
	klogf("[uKernel] Platform early init done\n");

	//  Handles further platform initialization tasks
	if(const auto err = arch::platform_init(); err != core::Error::Ok) {
		//  FIXME: Better error handling, break into debugger?
		kerrorf("[uKernel] Platform init failed with error code: {}\n", static_cast<size_t>(err));
		kpanic();
	}
	klogf("[uKernel] Platform init done\n");

	//  Initialize required kernel subsystems
	KHeap::instance().init();
	klogf("[uKernel] Early init done, time passed: {}ms\n", PIT::milliseconds());

	//  Prepare the next stage of initialization
	//  Late init is called within a multitasking environment, so it's
	//  safe to use more advanced locking primitives
	auto thread =
	        Process::create_with_main_thread(gen::String { "late_init" }, Process::kerneld(), core::start::late_init);
	this_cpu().scheduler().run_here(thread);
	this_cpu().scheduler().bootstrap();
	kpanic();
}

/**
 * Perform late initialization of device drivers.
 * Late init is called within a proper multitasking environment within
 * a separate late_init kernel process.
 */
[[noreturn, maybe_unused]] void core::start::late_init() {
	(void)driver::ide::init();

	klogf("[start::late_init] Late init completed\n");
	this_cpu().scheduler().sleep();
	//  We should never be awoken, but put a safe guard anyway
	while(true)
		;
}
