#include "Start.hpp"
#include <Arch/Interface.hpp>
#include <Arch/x86_64/PIT.hpp>
#include <Debug/kpanic.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>
#include <SMP/SMP.hpp>

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

	//  Initialize the kernel subsystems
	KHeap::instance().init();
	klogf("[uKernel] Init done, time passed: {}ms\n", PIT::milliseconds());
	this_cpu().scheduler().bootstrap();
	kpanic();
}
