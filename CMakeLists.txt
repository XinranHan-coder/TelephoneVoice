cmake_minimum_required(VERSION 3.22)
project(TelephoneVoice VERSION 1.0.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 JUCE 子目录，请确保路径正确
add_subdirectory(JUCE)

# 定义插件目标
juce_add_plugin(TelephoneVoice
    COMPANY_NAME "YourName"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Sing
    PLUGIN_CODE Tvce
    FORMATS VST3 Standalone
    PRODUCT_NAME "TelephoneVoice")

juce_generate_juce_header(TelephoneVoice)

# 添加源文件
target_sources(TelephoneVoice PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

# 链接必需的 JUCE 模块
target_link_libraries(TelephoneVoice PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)