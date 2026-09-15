cmake_minimum_required(VERSION 3.22)
project(shiftbench LANGUAGES CXX)

# Separate g++ (MSYS2 ucrt64) tree: functions.hpp uses __builtin_memcpy, and g++ is closest to the
# arm-none-eabi firmware toolchain. Configure with -DCMAKE_CXX_COMPILER=g++ (see .claude/skills/build).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/../..)
file(GLOB VARIANT_SOURCES CONFIGURE_DEPENDS ${REPO}/variants/*.cpp)

add_executable(shiftbench main.cpp variants.cpp ${REPO}/shift.cpp ${VARIANT_SOURCES})

target_include_directories(shiftbench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../compat
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${REPO}
    ${REPO}/variants
    ${REPO}/isl/include)

# isl expects Sample from the build (its CMake passes Sample=$ENV{SAMPLE_TYPE}); constants.hpp
# redefines it identically, which is legal.
target_compile_definitions(shiftbench PRIVATE Sample=float)
target_compile_options(shiftbench PRIVATE -O2 -Wall -Wextra -Wno-unused-parameter)
target_link_options(shiftbench PRIVATE -static)
set_target_properties(shiftbench PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# C-API DLL of the firmware Shift for the MSVC listening plugin (tools/listen_plugin). Static runtime so the
# DLL has no MinGW dependencies next to it. Rebuild after changing shift.hpp/shift.cpp:
#   cmake --build build/bench --target shift_capi
add_library(shift_capi SHARED ${REPO}/tools/listen_plugin/shift_capi.cpp ${REPO}/shift.cpp)
target_include_directories(shift_capi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../compat
    ${REPO}
    ${REPO}/isl/include
    ${REPO}/tools/listen_plugin)
target_compile_definitions(shift_capi PRIVATE Sample=float)
target_compile_options(shift_capi PRIVATE -O2 -Wall -Wextra -Wno-unused-parameter)
target_link_options(shift_capi PRIVATE -static -static-libgcc -static-libstdc++)
set_target_properties(shift_capi PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
