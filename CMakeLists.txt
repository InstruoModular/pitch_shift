cmake_minimum_required(VERSION 3.22)
project(vst_analysis VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC tree: JUCE headless VST3 host only. Shiftbench is a separate g++ tree (tools/shiftbench).
add_subdirectory(JUCE)
add_subdirectory(tools/host)
add_subdirectory(tools/vst2host)
add_subdirectory(tools/listen_plugin)
