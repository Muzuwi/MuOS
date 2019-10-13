#include <kernel/tty.h>
#include <kernel/io/ps2.h>
#include <kernel/cpuid.h>
#include <string.h>
#include <stdio.h>

void kernel_entrypoint(void){
    tty_init();
    printf("MuOS Kernel Entrypoint\n");
    {
        unsigned int msw = cpuid_get_msw();
        printf("[cpuid] CPU: %s\n", cpuid_get_cpu_brandstring());
        printf("[cpuid] MSW: %x\n", msw);
        if(msw & 1){
            printf("[cpuid] System is in protected mode\n");
        } else {
            printf("[cpuid] System is in real mode\n");
        }
    }
    //kernel_libc_test();
    ps2_init_controller();

}
