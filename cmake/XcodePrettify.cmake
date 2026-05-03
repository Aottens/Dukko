# Xcode source-tree organization.
# Vendored from sudara/cmake-includes (MIT) — content unchanged except commentary.

# No, we don't want our source buried in extra nested folders
set_target_properties(SharedCode PROPERTIES FOLDER "")

# The Xcode source tree should still look like the source tree
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source PREFIX "" FILES ${SourceFiles})

# Tucks the Plugin varieties into a "Targets" folder and generates an Xcode Scheme manually.
# Xcode scheme generation is turned off globally to limit noise from other targets.
foreach (target ${FORMATS} "All")
    if (TARGET ${PROJECT_NAME}_${target})
        set_target_properties(${PROJECT_NAME}_${target} PROPERTIES
            FOLDER "Targets"
            XCODE_GENERATE_SCHEME ON)

        # Set the default executable that Xcode will open on build.
        # NOTE: When using CPM-fetched JUCE, the AudioPluginHost is not built by
        # default. Either build it manually from the JUCE source dir, or remove
        # this line if you don't want the Xcode warning.
        if ((NOT target STREQUAL "All") AND (NOT target STREQUAL "Standalone"))
            if (DEFINED JUCE_SOURCE_DIR)
                set_target_properties(${PROJECT_NAME}_${target} PROPERTIES
                    XCODE_SCHEME_EXECUTABLE "${JUCE_SOURCE_DIR}/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app")
            endif ()
        endif ()
    endif ()
endforeach ()

if (TARGET Assets)
    set_target_properties(Assets PROPERTIES FOLDER "Targets")
endif ()
