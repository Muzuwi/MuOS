#       Main CMakeLists for the entire OS

project(MuOS)
cmake_minimum_required(VERSION 3.15)

#  Compile modules
add_subdirectory(LibC/)
add_subdirectory(Kernel/)
add_subdirectory(Library/LibGeneric/)
add_subdirectory(Userland/)

#  Toolchain configuration
include (Toolchain/CMakeLists.txt)

#  Creating a bootable ISO
find_program(GRUB_MKRESCUE grub-mkrescue)
if(NOT GRUB_MKRESCUE)
    message(WARNING "-- No grub-mkrescue detected! Creating a bootable ISO won't be possible")
else()
    message("-- Found grub-mkrescue - ${GRUB_MKRESCUE}")
endif()

if(GRUB_MKRESCUE)
    add_custom_target( BootableISO
        DEPENDS KernelELF
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Root
        COMMAND ${CMAKE_COMMAND} -E make_directory isodir/boot/
        COMMAND ${CMAKE_COMMAND} -E copy uKernel.bin isodir/boot/
        COMMAND ${GRUB_MKRESCUE} -o MuOS.iso isodir/
    )
endif()
