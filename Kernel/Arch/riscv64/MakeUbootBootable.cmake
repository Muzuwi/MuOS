find_program(MKIMAGE mkimage)
if (NOT MKIMAGE)
    message(WARNING "No mkimage from u-boot-tools found! Creating a bootable image won't be possible")
    return()
endif()

find_program(OBJCOPY riscv64-elf-objcopy)
if (NOT OBJCOPY)
    message(WARNING "No objcopy found! Creating a bootable image won't be possible")
    return()
endif()

message(STATUS "Found mkimage: ${MKIMAGE}")
message(STATUS "Found objcopy: ${OBJCOPY}")

add_custom_command(
    COMMENT "Generating Boot0.bin.."
    OUTPUT Boot0.bin
    DEPENDS Boot0
    COMMAND ${OBJCOPY} --gap-fill 0x00 --pad-to 0x80410000 -O binary $<TARGET_FILE:Boot0> Boot0.bin
)

add_custom_command(
    COMMENT "Generating uKernel.bin.."
    OUTPUT uKernel.bin
    DEPENDS KernelELF
    COMMAND ${OBJCOPY} --gap-fill 0x00 -O binary $<TARGET_FILE:KernelELF> uKernel.bin
)

add_custom_command(
    COMMENT "Generating KernelWithBoot0.bin.."
    OUTPUT KernelWithBoot0.bin
    DEPENDS Boot0.bin
    DEPENDS uKernel.bin
    COMMAND cat Boot0.bin uKernel.bin >KernelWithBoot0.bin
)

add_custom_target(BootableUImage
    COMMENT "Generating uKernel.uImage.."
    OUTPUT uKernel.uImage
    DEPENDS KernelWithBoot0.bin
    COMMAND ${MKIMAGE} -A riscv -O linux -T kernel -C none -a 0x80400000 -e 0x80400000 -n uKernel -d KernelWithBoot0.bin uKernel.uImage
    )

deploy_binary(BootableUImage
    GENERATED uKernel.uImage
    NAME "uKernel.${MU_MACHINE}.uImage"
    )