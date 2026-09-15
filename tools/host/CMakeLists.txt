juce_add_console_app(vsthost PRODUCT_NAME "vsthost")

target_sources(vsthost PRIVATE vsthost.cpp)
target_include_directories(vsthost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

target_compile_definitions(vsthost PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(vsthost PRIVATE
    juce::juce_audio_processors_headless
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
