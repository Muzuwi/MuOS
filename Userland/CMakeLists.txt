include(${CMAKE_SOURCE_DIR}/Toolchain/CMakeLists.txt)

#set(CMAKE_C_LINK_EXECUTABLE
#    "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> ${CMAKE_BINARY_DIR}/Userland/UsrCRTi.o ${CMAKE_BINARY_DIR}/Userland/UsrCRTbegin.o ${CMAKE_BINARY_DIR}/Userland/UsrCRT0.o <OBJECTS> <LINK_LIBRARIES> ${CMAKE_BINARY_DIR}/Userland/UsrCRTend.o ${CMAKE_BINARY_DIR}/Userland/UsrCRTn.o -o <TARGET>"
#    )

#set(CMAKE_CXX_LINK_EXECUTABLE
#    "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> ${CMAKE_BINARY_DIR}/Userland/UsrCRTi.o ${CMAKE_BINARY_DIR}/Userland/UsrCRTbegin.o ${CMAKE_BINARY_DIR}/Userland/UsrCRT0.o <OBJECTS> <LINK_LIBRARIES> ${CMAKE_BINARY_DIR}/Userland/UsrCRTend.o ${CMAKE_BINARY_DIR}/Userland/UsrCRTn.o -o <TARGET>"
#    )

include_guard()


add_subdirectory(Test/)
