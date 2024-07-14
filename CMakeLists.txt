# Root CMakeLists for the OS
cmake_minimum_required(VERSION 3.17)
project(MuOS LANGUAGES NONE)

include(CMake/ConfigureCMake.cmake)

#  Compile modules
add_subdirectory(Kernel/)
add_subdirectory(LibC/)
add_subdirectory(Library/LibGeneric/)
