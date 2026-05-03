# Embeds anything in assets/ as JUCE BinaryData on the Assets target.
# Vendored from sudara/cmake-includes (MIT) — content unchanged.
#
# HEADS UP: anything you stick in the assets folder gets included in the binary.
# Easy DX, but will bloat the binary if unused files live in there.

file(GLOB_RECURSE AssetFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
list (FILTER AssetFiles EXCLUDE REGEX "/\\.DS_Store$") # We don't want the .DS_Store on macOS

# Setup our binary data as a target called Assets
juce_add_binary_data(Assets SOURCES ${AssetFiles})

# Required for Linux happiness:
# See https://forum.juce.com/t/loading-pytorch-model-using-binarydata/39997/2
set_target_properties(Assets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
