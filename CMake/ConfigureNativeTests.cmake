# This module is used for setting up native tests utilizing Catch2.
# A native executable can be linked with the Catch2::Catch2 target
# to use Catch2 in the test sources.
# Remember to call `catch_discover_tests` to make the test cases
# available via CTest, otherwise they won't be run automatically.

include(FetchContent)
FetchContent_Declare(
    ExternCatch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG 62fd660583d3ae7a7886930b413c3c570e89786c
)
FetchContent_MakeAvailable(ExternCatch2)

# Append the cmake modules from catch2 for catch_discover_tests
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/contrib)
include(Catch)
include(CTest)
