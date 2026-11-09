
cmake_minimum_required(VERSION 3.22)

project(Syntaziser VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/JUCE)

juce_add_gui_app(Syntaziser
    PRODUCT_NAME "Syntaziser"
)

target_sources(Syntaziser PRIVATE
    src/Main.cpp
)

target_compile_definitions(Syntaziser PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(Syntaziser PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
