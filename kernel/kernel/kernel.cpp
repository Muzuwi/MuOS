#include <stdio.h>
#include <string.h>
#include <kernel/tty.h>
#include <arch/i386/i8042.hpp>

namespace uKernel {
	extern "C" void kernel_entrypoint();
};

/*
	Main kernel entrypoint
*/
extern "C" void uKernel::kernel_entrypoint(){
	tty_init();
	printf("[uKernel] uKernel booting\n");

	i8042::init_controller();

	printf("[uKernel] Main loop\n");
	while(true){
		// printf(".");
	}
}