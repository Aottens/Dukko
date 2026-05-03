cmake_minimum_required(VERSION 3.25)

# Dukko: lock to Apple Silicon arm64 + macOS 11 Big Sur deployment target.
# Plan 01 ships v1 as arm64-only (PROJECT.md "Platform (v1)"); plan 02+ may
# add x86_64 to this list once a Universal binary is wanted.
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

# This tells cmake we have goodies in the /cmake folder.
# Plan 02 vendored cmake-includes (sudara/cmake-includes, MIT) inline as
# Dukko*.cmake (DukkoVersion, DukkoMacOS, DukkoLog, DukkoIPP) plus the
# unrenamed helpers (CPM, JUCEDefaults, SharedCodeDefaults, Assets,
# XcodePrettify, Tests, Benchmarks, GitHubENV). See LICENSES.md.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (DukkoVersion)

# Modern concise way to add dependencies to your project
include (CPM)

# Configures universal binaries and decides which version of macOS to support
include(DukkoMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# ==============================================================================
# PERMANENT — DO NOT CHANGE — see D-01..D-04 in .planning/phases/01-build-foundation-ci/01-CONTEXT.md
#
# These four values are baked into the plugin binary and the VST3/CLAP manifests.
# Once any Bitwig (or other host) project saves a reference to these IDs, changing
# them will make every project that ever loaded Dukko report "plugin missing":
#   - PROJECT_NAME / PRODUCT_NAME    = "Dukko"                    (D-05)
#   - COMPANY_NAME (manufacturer)    = "Dukko Audio"              (D-01)
#   - PLUGIN_MANUFACTURER_CODE       = "Dukk"                     (D-02)
#   - PLUGIN_CODE                    = "Dkk1"                     (D-03)
#   - BUNDLE_ID / CLAP_ID            = "com.dukkoaudio.dukko"     (D-04)
# ==============================================================================

# Internal name of the project and the name of JUCE's shared code target.
# Cannot have spaces. PRODUCT_NAME (below) is what the DAW displays.
set(PROJECT_NAME "Dukko")

# What the DAW displays. For Dukko, both are the same — the product is just "Dukko".
set(PRODUCT_NAME "Dukko")

# Used for the macOS bundle name, installers, and the AU/VST3/CLAP manufacturer
# field. Bitwig groups plugins by manufacturer in its browser using this string.
set(COMPANY_NAME "Dukko Audio")

# macOS bundle identifier (and code-signing identity). Also reused as CLAP_ID
# when clap-juce-extensions is wired in plan 02.
set(BUNDLE_ID "com.dukkoaudio.dukko")

# Plugin formats. v1 ships VST3 only via this list; CLAP is wired separately
# through clap-juce-extensions in plan 02. AU/AAX/Standalone are explicitly
# out of scope per PROJECT.md "Out of Scope".
set(FORMATS VST3)

# Force JUCE to work with sccache
# instruct MSVC to embed debug info (/Z7) instead of emitting a .pdb
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# Log build environment info (after project() so compiler info is available)
include(DukkoLog)

# ==============================================================================
# Runtime dependency pins (D-15, D-16). LICENSES.md tracks each entry.
#
# - JUCE                 → tag 8.0.12 (released 2024-12-16; latest stable JUCE 8.0.x)
# - clap-juce-extensions → main HEAD SHA at clone time (no release tags exist).
#                          Resolved via `git ls-remote` on 2026-05-03; bump
#                          deliberately after auditing upstream changes.
# - chowdsp_utils        → tag v2.4.0 (released 2025-11-29; BSD-3-Clause).
#                          Phase 1 fetches the dep so LICENSES.md is locked
#                          from day one (D-15 rationale). Phase 2 will link
#                          chowdsp::chowdsp_plugin_state when versioned state
#                          recall lands — no CMake change needed at that point.
#
# GIT_SHALLOW FALSE on clap-juce-extensions because shallow clone fails for
# non-tag SHAs. JUCE + chowdsp pin to tags so shallow clone is safe.
# ==============================================================================

CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)

# clap-juce-extensions main @ 2026-05-03 — see TBD-during-execution #2 in
# .planning/phases/01-build-foundation-ci/01-RESEARCH.md.
CPMAddPackage(
    NAME clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG e8de9e8571626633b8541a54c2406fccc4272767
    GIT_SHALLOW FALSE
)

CPMAddPackage(
    NAME chowdsp_utils
    GIT_REPOSITORY https://github.com/Chowdhury-DSP/chowdsp_utils.git
    GIT_TAG v2.4.0
    GIT_SHALLOW TRUE
)
# Phase 2 will link chowdsp::chowdsp_plugin_state once versioned-state work begins.

# Melatonin Inspector — JUCE component inspector. CPM-fetched (was a git submodule
# in upstream Pamplejuce). The repo has no release tags, so we pin to a specific
# main HEAD SHA captured 2026-05-03. GIT_SHALLOW FALSE because shallow clone of
# a SHA (not a tag/branch) fails. DOWNLOAD_ONLY because the upstream CMakeLists
# assumes its directory is named "melatonin_inspector" (CPM extracts into
# "melatonin_inspector-src"); we call juce_add_module directly below to bypass.
CPMAddPackage(
    NAME melatonin_inspector
    GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
    GIT_TAG 9e91e4e3d6cc41688c8d2108ef7ed33c1a90dcc9
    GIT_SHALLOW FALSE
    DOWNLOAD_ONLY YES
)

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/my_module)

# Manually register melatonin_inspector as a JUCE module from the CPM-downloaded
# source dir. Done here (not via the upstream CMakeLists) because the upstream
# script asserts the directory basename equals "melatonin_inspector". JUCE's
# juce_add_module helper also requires the dir basename to equal the module ID,
# so we symlink melatonin_inspector → melatonin_inspector-src once at configure
# time and pass the symlink path.
set(_mi_link "${CMAKE_BINARY_DIR}/_deps/melatonin_inspector")
if (NOT EXISTS "${_mi_link}")
    file(CREATE_LINK "${melatonin_inspector_SOURCE_DIR}" "${_mi_link}" SYMBOLIC)
endif ()
juce_add_module("${_mi_link}")

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    # Icons for the standalone app
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

    # PERMANENT — see D-01..D-04 comment block above.
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to /Users/yourname/Library/Audio/Plug-Ins/
    # D-09: KEEP TRUE — auto-install into ~/Library/Audio/Plug-Ins/VST3/ on every
    # local build so Bitwig picks it up without a manual copy step.
    COPY_PLUGIN_AFTER_BUILD TRUE

    # PERMANENT — D-02. Four-character manufacturer code; first char uppercase.
    PLUGIN_MANUFACTURER_CODE Dukk

    # PERMANENT — D-03. Four-character plugin code; at least one uppercase char.
    PLUGIN_CODE Dkk1
    FORMATS "${FORMATS}"

    # The name of your final executable. This is how it's listed in the DAW.
    PRODUCT_NAME "${PRODUCT_NAME}")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

# Wire CLAP target onto the JUCE plugin (BUILD-03). The clap_juce_extensions
# CPM block above defines this macro. CLAP_ID literal must match BUNDLE_ID
# (and PROJECT_NAME literal must match) per D-04 — written out in literal form
# both so the plan-02 grep gates match and so a future reader sees the canonical
# CLAP plugin id at the wiring site without chasing variables. Any edit here
# must keep these in lockstep with the PERMANENT block above.
clap_juce_extensions_plugin(
    TARGET Dukko
    CLAP_ID "com.dukkoaudio.dukko"
    CLAP_FEATURES audio-effect mixing
)

# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)


# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE

    # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
    JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0

    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # lets the app known if we're Debug or Release
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"

    # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
    PRODUCT_NAME_WITHOUT_VERSION="Dukko"
)

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# In Dukko's CMake layout, modules link to SharedCode with INTERFACE visibility
# so JUCE plugin targets and the Tests target both pick them up.
target_link_libraries(SharedCode
    INTERFACE
    Assets
    melatonin_inspector
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# IPP support, comment out to disable
include(DukkoIPP)

# Everything related to the tests target
include(Tests)

# A separate target for Benchmarks (keeps the Tests target fast)
include(Benchmarks)

# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)
