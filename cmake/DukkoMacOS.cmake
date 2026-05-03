# Dukko-specific macOS configuration.
#
# NOTE: CMakeLists.txt at the repo root sets CMAKE_OSX_ARCHITECTURES=arm64
# and CMAKE_OSX_DEPLOYMENT_TARGET=11.0 BEFORE include(DukkoMacOS) so we do
# not override them here. Upstream Pamplejuce shipped 10.14 + auto-universal
# binaries on CI; Dukko v1 is arm64-only on macOS 11.0+ per PROJECT.md
# "Platform (v1)".
#
# Vendored from sudara/cmake-includes (MIT) — minimal, kept only the Xcode
# scheme-noise toggle that the rest of the helpers expect.

# By default we don't want Xcode schemes to be made for modules, etc
set(CMAKE_XCODE_GENERATE_SCHEME OFF)
