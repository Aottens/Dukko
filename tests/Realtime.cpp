#include "AllocationGuard.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cmath>
#include <thread>

static void set (juce::AudioProcessor& plugin, const juce::String& id, float value0to1)
{
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && ranged->getParameterID() == id)
            ranged->setValueNotifyingHost (value0to1);
}

// Escapes through a volatile pointer, so the compiler cannot elide the new/delete pair.
static void allocateOne()
{
    static int* volatile sink = nullptr;
    sink = new int (1);
    delete sink;
}

TEST_CASE ("dsp_06_allocation_guard_counts_only_armed_thread")
{
    {
        AllocationGuard guard;
        allocateOne();
        CHECK (guard.count() == 1);
    }

    allocateOne(); // disarmed: must not count anywhere
    std::atomic<bool> go { false };
    std::thread other ([&] { while (! go) {} allocateOne(); }); // created before the guard: std::thread allocates
    AllocationGuard guard;
    go = true;
    other.join();
    CHECK (guard.count() == 0);
}

TEST_CASE ("dsp_06_process_block_does_not_allocate")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;

    int allocations = 0;
    for (int block = 0; block < 100; ++block)
    {
        set (plugin, "depth", static_cast<float> (block % 7) / 6.0f);
        set (plugin, "mix", static_cast<float> (block % 5) / 4.0f);
        set (plugin, "bypass", block % 10 < 5 ? 0.0f : 1.0f);

        const int sizes[] = { 256, 1, 37 };
        buffer.setSize (2, sizes[block % 3], false, false, true);
        buffer.clear();

        AllocationGuard guard;
        plugin.processBlock (buffer, midi);
        allocations += guard.count();
    }
    CHECK (allocations == 0);
}

TEST_CASE ("dsp_07_process_block_flushes_denormals_and_restores_mode")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 1.0e-40f);
    REQUIRE (std::fpclassify (buffer.getSample (0, 0)) == FP_SUBNORMAL);

    juce::MidiBuffer midi;
    const auto before = juce::FloatVectorOperations::areDenormalsDisabled();
    plugin.processBlock (buffer, midi);
    CHECK (juce::FloatVectorOperations::areDenormalsDisabled() == before);

    int subnormal = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            subnormal += std::fpclassify (buffer.getSample (ch, i)) == FP_SUBNORMAL ? 1 : 0;
    CHECK (subnormal == 0);
}
