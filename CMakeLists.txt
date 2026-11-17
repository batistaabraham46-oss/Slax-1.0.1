cmake_minimum_required(VERSION 3.22)

project(Slax VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE is downloaded automatically the first time you configure the project.
# (Needs git + internet. To use a local copy instead, configure with
#  -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

set(UM_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND UM_FORMATS AU)
endif()

juce_add_plugin(Slax
    COMPANY_NAME                "UGM Audio"
    PRODUCT_NAME                "Slax"
    VERSION                     1.0.0
    PLUGIN_MANUFACTURER_CODE    Ugma
    PLUGIN_CODE                 Slax
    FORMATS                     ${UM_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    VST3_CATEGORIES             Fx Dynamics
    COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(Slax PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
    Params.h
    Presets.h
    UI.h)

target_compile_definitions(Slax PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Slax
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
