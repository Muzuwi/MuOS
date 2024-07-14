find_program(GRUB_MKRESCUE grub-mkrescue)
if (NOT GRUB_MKRESCUE)
    message(WARNING "No grub-mkrescue found! Creating a bootable ISO won't be possible")
    return()
endif()

message(STATUS "Found grub-mkrescue: ${GRUB_MKRESCUE}")

add_custom_target(BootableISO
    COMMENT "Generating bootable GRUB ISO"
    DEPENDS KernelELF
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Root
    COMMAND ${CMAKE_COMMAND} -E make_directory isodir/boot/
    COMMAND ${CMAKE_COMMAND} -E copy uKernel.bin isodir/boot/
    COMMAND ${GRUB_MKRESCUE} -o MuOS.iso isodir/
    )
