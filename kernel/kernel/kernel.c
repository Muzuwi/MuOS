#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

//  Buffer used to store the detected CPU name
static char cpu_brandstring_buff[48];


char* kernel_get_cpu_brandstring(){
    unsigned int pointer = 0;

    uint32_t regsTemp[4];
    for(size_t i = 0; i < 3; i++){
        uint32_t command = 0x80000002 + i;
        asm __volatile__(
            "mov %%eax, %0\n"
            "cpuid\n"
            "mov dword ptr [%1 + 0] , %%eax\n"
            "mov dword ptr [%1 + 4] , %%ebx\n"
            "mov dword ptr [%1 + 8] , %%ecx\n"
            "mov dword ptr [%1 + 12], %%edx"
            :
            : "r"(command), "r"(regsTemp)
            : "%eax", "%ebx", "%ecx", "%edx", "memory"
        );
        for(size_t part = 0; part < 4; part++){
            for(size_t block = 0; block < 4; block++){
                uint8_t byte = ((regsTemp[part] & ((0xFF) << (8 * block))) >> (8 * block));
                cpu_brandstring_buff[pointer++] = byte;
                if(byte == '\0') {
                    return &cpu_brandstring_buff[0];
                }
            }
        }
    }

    //  TODO: Error handling?
    return &cpu_brandstring_buff[0];
}

void kernel_entrypoint(void){
    tty_init();
    printf("MuOS Kernel Entrypoint\n");
    printf("Testing printf capabilities\n");
    printf("Integer: %i\n", 561);
    printf("Character: %c\n", 'a');
    printf("Hex: %x\n", 2147483647);
    printf("Hex (upper): %X\n", 2147483647);
    printf("String: %s\n", "Testing cool stuff");
    printf("Mix and match! int: %i, char: %c, hex: %x, str: %s\n", 1337, 'n', 420, "hello world!");
    printf("CPU: %s\n", kernel_get_cpu_brandstring());
    printf("Escaping test: \\a, \\\\, %%, \\%\n");
}
