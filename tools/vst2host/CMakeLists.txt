# Clean-room VST2 host (no Steinberg SDK needed). Windows only.
add_executable(vst2host vst2host.cpp)
target_include_directories(vst2host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(vst2host PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(vst2host PRIVATE /W4 /EHsc)
endif()
target_link_libraries(vst2host PRIVATE ole32 user32)
set_target_properties(vst2host PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/vst2host)
