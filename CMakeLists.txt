# Root CMakeLists for the OS
cmake_minimum_required(VERSION 3.17)
project(MuOS LANGUAGES NONE)

enable_testing()

#  Signal that this is the main kernel build
set(MU_BUILD_TYPE cross)

# Add the project CMake modules to the search path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

#  Compile modules
add_subdirectory(Kernel/)
add_subdirectory(LibC/)
add_subdirectory(Library/LibGeneric/)

#  Creating a bootable ISO
find_program(GRUB_MKRESCUE grub-mkrescue)
if (NOT GRUB_MKRESCUE)
    message(WARNING "-- No grub-mkrescue detected! Creating a bootable ISO won't be possible")
else ()
    message("-- Found grub-mkrescue - ${GRUB_MKRESCUE}")

    add_custom_target(BootableISO
        DEPENDS KernelELF
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Root
        COMMAND ${CMAKE_COMMAND} -E make_directory isodir/boot/
        COMMAND ${CMAKE_COMMAND} -E copy uKernel.bin isodir/boot/
        COMMAND ${GRUB_MKRESCUE} -o MuOS.iso isodir/
        )
endif ()
