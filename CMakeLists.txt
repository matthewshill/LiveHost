cmake_minimum_required(VERSION 3.22)

project(LiveHost VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/JUCE)

juce_add_gui_app(LiveHost
    PRODUCT_NAME "LiveHost"
    COMPANY_NAME "LiveHost"
    BUNDLE_ID "com.livehost.app"
    VERSION "${PROJECT_VERSION}"
    ICON_BIG ""
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE)

juce_generate_juce_header(LiveHost)

target_sources(LiveHost
    PRIVATE
        Source/Audio/AudioEngine.cpp
        Source/Audio/AudioEngine.h
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h)

target_compile_definitions(LiveHost
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:LiveHost,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(LiveHost
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
