# Compiler defaults for the SharedCode INTERFACE target.
# Vendored from sudara/cmake-includes (MIT) — switched cxx_std_23 → cxx_std_20
# per CLAUDE.md tech-stack ("C++20 ... no DSP cost; C++23 reduces toolchain margin, skip").

if (MSVC)
    # fast math and better simd support in RELEASE
    # https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
    target_compile_options(SharedCode INTERFACE $<$<CONFIG:RELEASE>:/fp:fast>)
    target_compile_options(SharedCode INTERFACE $<$<CONFIG:RELEASE>:/Ox>)
else ()
    # See the implications here:
    # https://stackoverflow.com/q/45685487
    target_compile_options(SharedCode INTERFACE $<$<CONFIG:RELEASE>:-Ofast>)
    target_compile_options(SharedCode INTERFACE $<$<CONFIG:RelWithDebInfo>:-Ofast>)
endif ()

# Tell MSVC to properly report what c++ version is being used
if (MSVC)
    target_compile_options(SharedCode INTERFACE /Zc:__cplusplus)
endif ()

# C++20 (CLAUDE.md tech-stack lock).
target_compile_features(SharedCode INTERFACE cxx_std_20)
