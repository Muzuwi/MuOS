# Root CMakeLists for the OS
cmake_minimum_required(VERSION 3.17)
project(MuOS LANGUAGES NONE)

include(CMake/ConfigureCMake.cmake)
find_package(MuToolchain REQUIRED)

add_subdirectory(Kernel/)
add_subdirectory(Library/LibC/)
add_subdirectory(Library/LibGeneric/)
add_subdirectory(Library/LibAllocator/)
add_subdirectory(Library/LibFormat/)
