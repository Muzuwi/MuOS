cmake_minimum_required(VERSION 3.17)

message(STATUS "Using host toolchain")

# When using the cross-compiler toolchains we'd have to modify some
# CMake variables to make sure that the typical C++ target functions
# (add_executable / add_library / ...) use the cross-compiler instead
# of the default host compiler (see: x86_64/Toolchain.cmake). CMake
# performs discovery of the host compiler on first call to project()
# with CXX language enabled, so this has to be done before the very
# first call to project() is made (which is what the cross-compiled
# toolchain definitions do).
#
# If we want to use the host toolchain, there's nothing to be done as CMake
# will already do the right thing and configure the host toolchain on its
# own on next call. As such, this file is intentionally a no-op.
