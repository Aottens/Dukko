cmake_minimum_required(VERSION 3.25)

# Apple Silicon only for v1 (PROJECT.md). CACHE without FORCE so -D overrides win.
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(Dukko VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE)
FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG e8de9e8571626633b8541a54c2406fccc4272767)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE clap-juce-extensions Catch2)

# PERMANENT (DECISIONS 2026-05-03): hosts store these in saved projects. Changing any of
# them makes every project that ever used Dukko report "plugin missing".
juce_add_plugin(Dukko
    COMPANY_NAME "Dukko Audio"
    BUNDLE_ID com.dukkoaudio.dukko
    PLUGIN_MANUFACTURER_CODE Dukk
    PLUGIN_CODE Dkk1
    FORMATS VST3
    PRODUCT_NAME Dukko
    COPY_PLUGIN_AFTER_BUILD TRUE)

clap_juce_extensions_plugin(TARGET Dukko
    CLAP_ID com.dukkoaudio.dukko
    CLAP_FEATURES audio-effect mixing)

# Plugin code shared by the plugin and the tests (no ODR clash: an INTERFACE library).
add_library(DukkoCode INTERFACE)
file(GLOB_RECURSE DUKKO_SOURCES CONFIGURE_DEPENDS source/*.cpp source/*.h)
target_sources(DukkoCode INTERFACE ${DUKKO_SOURCES})
target_include_directories(DukkoCode INTERFACE source)
target_compile_definitions(DukkoCode INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(DukkoCode INTERFACE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

option(DUKKO_ASAN "Build with AddressSanitizer" OFF)
if(DUKKO_ASAN)
    target_compile_options(DukkoCode INTERFACE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(DukkoCode INTERFACE -fsanitize=address)
endif()

target_link_libraries(Dukko PRIVATE DukkoCode)

enable_testing()
file(GLOB_RECURSE DUKKO_TESTS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(Tests ${DUKKO_TESTS})
target_compile_definitions(Tests PRIVATE $<TARGET_PROPERTY:Dukko,COMPILE_DEFINITIONS>)
target_link_libraries(Tests PRIVATE DukkoCode Catch2::Catch2WithMain)
include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(Tests DISCOVERY_MODE PRE_TEST)
