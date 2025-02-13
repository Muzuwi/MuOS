find_program(GRUB_MKRESCUE grub-mkrescue)
if (NOT GRUB_MKRESCUE)
    message(WARNING "No grub-mkrescue found! Creating a bootable ISO won't be possible")
    return()
endif()

message(STATUS "Found grub-mkrescue: ${GRUB_MKRESCUE}")

# Generate GRUB configuration based on build configuration.
# See Boot/grub.cfg.in for the template that is being populated.
set(BOOTABLE_ENTRY_NAME "MuOS")
set(BOOTABLE_EXECUTABLE "/boot/uKernel.elf")
configure_file(
    Boot/grub.cfg.in
    grub.cfg
    @ONLY
    )
add_custom_command(
    OUTPUT MuOS.iso
    BYPRODUCTS isodir/boot/grub/grub.cfg isodir/boot/grub/ isodir${BOOTABLE_EXECUTABLE} isodir/boot/ isodir/
    DEPENDS KernelELF grub.cfg
    COMMAND ${CMAKE_COMMAND} -E echo "[build] Generating bootable GRUB iso"
    COMMAND ${CMAKE_COMMAND} -E make_directory isodir/boot/grub/
    COMMAND ${CMAKE_COMMAND} -E copy grub.cfg isodir/boot/grub/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:KernelELF> "isodir${BOOTABLE_EXECUTABLE}"
    COMMAND ${GRUB_MKRESCUE} -o MuOS.iso isodir/
    )
add_custom_target(BootableISO
    DEPENDS MuOS.iso
    )
deploy_binary(BootableISO
    GENERATED MuOS.iso
    NAME "MuOS.${MU_MACHINE}.iso"
    )
