if(${MU_MACHINE} STREQUAL "x86_64")
    add_kernel_sources(MP/)
    add_kernel_sources(Object/)
    add_kernel_sources(Start/)
endif()

add_kernel_sources(Assert/)
add_kernel_sources(Error/)
add_kernel_sources(IO/)
add_kernel_sources(IRQ/)
add_kernel_sources(Log/)
add_kernel_sources(Mem/)
