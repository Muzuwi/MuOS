include(../../Toolchain/CMakeLists.txt)

add_custom_command(
    OUTPUT crt0.o
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -f elf64 ${CMAKE_CURRENT_SOURCE_DIR}/crt0.asm -o crt0.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/crt0.asm
)
add_custom_command(
    OUTPUT crti.o
    COMMAND ${CMAKE_ASM-ATT_COMPILER} -o crti.o ${CMAKE_CURRENT_SOURCE_DIR}/crti.S
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/crti.S
)
add_custom_command(
    OUTPUT crtn.o
    COMMAND ${CMAKE_ASM-ATT_COMPILER} -o crtn.o ${CMAKE_CURRENT_SOURCE_DIR}/crtn.S
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/crtn.S
)

add_custom_target(LibC-CRT0
    DEPENDS crt0.o
    )
add_custom_target(LibC-CRTi
    DEPENDS crti.o
    )
add_custom_target(LibC-CRTn
    DEPENDS crtn.o
    )
add_custom_target(LibC-CRT
    DEPENDS LibC-CRT0 LibC-CRTi LibC-CRTn
    )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/crt0.o
    DESTINATION lib
    )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/crti.o
    DESTINATION lib
    )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/crtn.o
    DESTINATION lib
    )
add_custom_command(TARGET LibC-CRT POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR})
