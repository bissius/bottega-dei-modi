cmake_minimum_required(VERSION 3.22)
project(BottegaDeiModi VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

# JUCE: se JUCE_DIR è impostato usa una copia locale, altrimenti la scarica (tag fisso)
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# Archivio delle culture incorporato nel binario
juce_add_binary_data(BottegaData SOURCES data/archivio.json)
set_target_properties(BottegaData PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Nucleo musicale: nessuna dipendenza dall'interfaccia (solo juce_core per JSON e stringhe)
add_library(bottega_core INTERFACE)
target_sources(bottega_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Archive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Theory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Song.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Sequencer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/MidiOut.cpp)
target_include_directories(bottega_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/core)

# ---------- Plugin (VST3, AU su Mac) + app standalone ----------
set(BOTTEGA_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND BOTTEGA_FORMATS AU)
endif()

juce_add_plugin(Bottega
    COMPANY_NAME "Bissio"
    PRODUCT_NAME "Bottega dei Modi"
    BUNDLE_ID "it.bissio.bottegadeimodi"
    PLUGIN_MANUFACTURER_CODE Biss
    PLUGIN_CODE Btga
    FORMATS ${BOTTEGA_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

target_sources(Bottega PRIVATE
    synth/Engine.cpp
    plugin/PluginProcessor.cpp
    plugin/PluginEditor.cpp
    plugin/Lanes.cpp)
target_include_directories(Bottega PRIVATE synth plugin)
target_compile_definitions(Bottega PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(Bottega
    PRIVATE bottega_core BottegaData
            juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---------- Test del nucleo (girano in CI su ogni piattaforma) ----------
juce_add_console_app(BottegaTests PRODUCT_NAME "BottegaTests")
target_sources(BottegaTests PRIVATE tests/CoreTests.cpp)
target_compile_definitions(BottegaTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(BottegaTests PRIVATE bottega_core BottegaData juce::juce_core
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# prova senza finestra: suona 12 secondi, conta il MIDI, salva una schermata dell'interfaccia
juce_add_console_app(BottegaSmoke PRODUCT_NAME "BottegaSmoke")
target_sources(BottegaSmoke PRIVATE tests/Smoke.cpp synth/Engine.cpp plugin/PluginProcessor.cpp plugin/PluginEditor.cpp plugin/Lanes.cpp)
target_include_directories(BottegaSmoke PRIVATE plugin synth core)
target_compile_definitions(BottegaSmoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Bottega dei Modi")
target_link_libraries(BottegaSmoke PRIVATE bottega_core BottegaData juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
