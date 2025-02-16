cmake_minimum_required(VERSION 3.17)
project(RiscvBoot0 LANGUAGES CXX ASM)

set(BOOT0_SOURCES
    Boot0.cpp
    Boot0.s
    $<$<BOOL:${CONFIG_BOOT0_BOOTCON_SIFIVE}>:BootConsole_SiFive.cpp>
    $<$<BOOL:${CONFIG_BOOT0_BOOTCON_16550}>:BootConsole_16550.cpp>
    BootConsole.cpp
    DeviceTree.cpp
    TinyMM.cpp
    )
add_executable(Boot0 ${BOOT0_SOURCES})
target_compile_options(Boot0
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS} -fno-stack-protector>
    )
target_compile_definitions(Boot0
    PRIVATE ARCH_IS_RISCV64=1
    )
target_include_directories(Boot0
    PRIVATE ${CMAKE_SOURCE_DIR}/Kernel/
    )
target_link_libraries(Boot0
    PRIVATE LibAllocator
    PRIVATE LibFormat
    PRIVATE LibGeneric
    )
target_link_options(Boot0
    PRIVATE -T ${PROJECT_SOURCE_DIR}/Linkscript.ld
    )
add_custom_command(TARGET Boot0
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/Root/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Boot0> ${CMAKE_SOURCE_DIR}/Root/Boot0.elf
    COMMENT "Copying kernel executable to Root directory ${CMAKE_SOURCE_DIR}/Root "
    )
