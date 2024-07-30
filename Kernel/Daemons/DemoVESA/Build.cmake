if(MU_MACHINE STREQUAL "x86_64" AND KERNEL_HACKS_VESADEMO)
    add_kernel_sources(
        DemoVESA.cpp
    )
    target_compile_definitions(KernelELF
        PRIVATE KERNEL_HACKS_VESADEMO=1
    )
endif()