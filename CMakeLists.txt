cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Binaires universels Mac")
project(VibratoMXA VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# JUCE : submodule du depot si initialise, sinon dossier voisin partage (suite MXA).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE JUCE)
else()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE JUCE)
endif()

juce_add_plugin(VibratoMXA
    COMPANY_NAME "MXAudio"
    BUNDLE_ID com.n2xlabs.vibratomxa
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE N2xL
    PLUGIN_CODE Vbmx
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "VibratoMXA"
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(VibratoMXA PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VibratoEngine.cpp
    Source/ManualStyle.cpp
    Source/DeviationPlot.cpp
    Source/SchematicDiagram.cpp)

# Polices embarquees du monde "Service Manual" (voir Assets/LICENSES.md).
juce_add_binary_data(VibratoMXAAssets SOURCES
    Assets/RoutedGothic-Regular.ttf
    Assets/RoutedGothicWide-Regular.ttf
    Assets/CourierPrime-Regular.ttf
    Assets/CourierPrime-Bold.ttf)

# Outil de capture hors-ecran (revue de design uniquement, jamais livre).
juce_add_console_app(VibratoMXASnapshot PRODUCT_NAME "VibratoMXASnapshot")
target_sources(VibratoMXASnapshot PRIVATE
    Tools/SnapshotMain.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VibratoEngine.cpp
    Source/ManualStyle.cpp
    Source/DeviationPlot.cpp
    Source/SchematicDiagram.cpp)
target_compile_definitions(VibratoMXASnapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(VibratoMXASnapshot PRIVATE
    VibratoMXAAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics)

target_compile_definitions(VibratoMXA PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VibratoMXA PRIVATE
    VibratoMXAAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
