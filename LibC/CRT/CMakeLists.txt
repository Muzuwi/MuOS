include(../../Toolchain/CMakeLists.txt)

add_custom_target(LibC_CRT0
                  COMMAND ${CMAKE_ASM_NASM_COMPILER} -f elf64 ${CMAKE_SOURCE_DIR}/LibC/CRT/crt0.asm -o ${CMAKE_CURRENT_BINARY_DIR}/crt0.o
                  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/crt0.o
                  )
add_custom_target(LibC_CRTi
                  COMMAND ${CMAKE_ASM-ATT_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/LibC_CRTi.o ${CMAKE_SOURCE_DIR}/LibC/CRT/crti.S
                  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/LibC_CRTi.o
                  )
add_custom_target(LibC_CRTn
                  COMMAND ${CMAKE_ASM-ATT_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/LibC_CRTn.o ${CMAKE_SOURCE_DIR}/LibC/CRT/crtn.S
                  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/LibC_CRTn.o
                  )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/crt0.o
        DESTINATION lib
        )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibC_CRTi.o
        DESTINATION lib
        RENAME crti.o
        )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibC_CRTn.o
        DESTINATION lib
        RENAME crtn.o
        )

add_custom_command(TARGET LibC_CRT0 POST_BUILD
                   COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR})
