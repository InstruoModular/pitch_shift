# One-knob listening-test plugin around the firmware Shift. MSVC tree.
#
# The DSP is NOT compiled here: shift.cpp/isl need GCC/Clang, so they are built into shift_capi.dll by the g++
# tree (`cmake --build build/bench --target shift_capi`) and loaded at runtime (see shift_capi.h). Post-build steps
# copy that DLL next to the VST3 module and the Standalone exe.
juce_add_plugin(ShiftListen
    PRODUCT_NAME "Shift Listen"
    COMPANY_NAME "vst_analysis"
    PLUGIN_MANUFACTURER_CODE Vsta
    PLUGIN_CODE Shft
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SHIFT_CAPI_DLL "${REPO}/build/bench/shift_capi.dll" CACHE FILEPATH "GCC-built firmware Shift DLL (target shift_capi)")

juce_generate_juce_header(ShiftListen)
target_sources(ShiftListen PRIVATE ShiftListen.cpp)
target_include_directories(ShiftListen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(ShiftListen PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ShiftListen
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Always-run sync target: builds both formats, then copies the current shift_capi.dll next to each. A POST_BUILD step
# alone is not enough -- when only shift.cpp changed the plugin doesn't relink, POST_BUILD never runs, and the bundle
# keeps the OLD DSP (this happened once; the smoke test caught it). Build this target, not the format targets.
add_custom_target(ShiftListen_SyncDll ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHIFT_CAPI_DLL}" "$<TARGET_FILE_DIR:ShiftListen_VST3>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHIFT_CAPI_DLL}" "$<TARGET_FILE_DIR:ShiftListen_Standalone>"
    COMMENT "Syncing shift_capi.dll into Shift Listen VST3 and Standalone"
    VERBATIM)
add_dependencies(ShiftListen_SyncDll ShiftListen_VST3 ShiftListen_Standalone)
