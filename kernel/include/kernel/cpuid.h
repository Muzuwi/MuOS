#ifndef KERNEL_CPUID_H
#define KERNEL_CPUID_H

#include <stddef.h>
#include <stdint.h>

/*struct cr0_t {
	unsigned int pe_enable : 1;
	unsigned int coprocessor : 1;
	unsigned int emulation : 1;
	unsigned int taskswitched : 1;
	unsigned int extension_type : 1;
	unsigned int numeric_error : 1;
	unsigned int : 10;
	unsigned int write_protect : 1;
	unsigned int : 1;
	unsigned int alignment_mask : 1;

}*/

const char* cpuid_get_cpu_brandstring();
unsigned int cpuid_get_msw();

#endif