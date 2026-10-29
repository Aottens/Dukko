#include "FakePlayHead.h"
#include <PluginProcessor.h>
#include <dsp/Curve.h>
#include <dsp/Duck.h>
#include <dsp/Sync.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr int half = 1, quarter = 4; // division indices (SYNC-02)

// Gain at one sample of a block, depth and mix 100 % unless given, the sync active and settled.
static float gainAt (double ppq, int sample, float depth = 1.0f)
{
    dsp::Sync sync;
    sync.prepare (48000.0);
    dsp::Duck duck;
    duck.setTargets (depth, 1.0f);
    duck.setActive (true);
    duck.prepare (48000.0);
    sync.setBlock ({ ppq, 120.0, true }, quarter);
    REQUIRE (sync.isActive());
    float g = 1.0f;
    for (int i = 0; i <= sample; ++i)
        g = duck.next (dsp::Curve::sine().value (sync.phase (i)));
    return g;
}

TEST_CASE ("dsp_01_sine_is_fully_ducked_on_the_beat")
{
    CHECK (dsp::Curve::sine().value (0.0) == Catch::Approx (1.0f));
    CHECK (dsp::Curve::sine().value (0.25) == Catch::Approx (0.5f));
    CHECK (dsp::Curve::sine().value (0.5) == Catch::Approx (0.0f).margin (1.0e-6));
    CHECK (dsp::Curve::sine().value (0.75) == Catch::Approx (0.5f));
}

TEST_CASE ("dsp_01_gain_at_known_ppq_positions")
{
    CHECK (gainAt (0.0, 0) == Catch::Approx (0.0f).margin (1.0e-6));
    CHECK (gainAt (0.25, 0) == Catch::Approx (0.5f));
    CHECK (gainAt (0.5, 0) == Catch::Approx (1.0f));
    CHECK (gainAt (0.0, 12000) == Catch::Approx (1.0f));
    CHECK (gainAt (0.0, 0, 0.5f) == Catch::Approx (0.5f));
}

TEST_CASE ("dsp_01_activity_ramps_over_10_ms")
{
    dsp::Duck duck;
    duck.setTargets (1.0f, 1.0f);
    duck.prepare (48000.0);
    CHECK (duck.next (1.0f) == Catch::Approx (1.0f)); // inactive after prepare

    dsp::Sync sync;
    sync.prepare (48000.0);
    for (bool playing : { true, false })
    {
        sync.setBlock ({ 0.0, 1.0, playing }, quarter);
        duck.setActive (sync.isActive());
        float previous = playing ? 1.0f : 0.0f, gain = previous;
        for (int i = 0; i < 480; ++i)
        {
            gain = duck.next (dsp::Curve::sine().value (sync.phase (i)));
            CHECK (std::abs (gain - previous) <= 1.0f / 480.0f + 1.0e-6f);
            previous = gain;
        }
        CHECK (gain == Catch::Approx (playing ? 0.0f : 1.0f).margin (1.0e-3)); // Curve::sine() is 1 - 1.3e-4 there
    }
}

static void setChoice (juce::AudioProcessor& plugin, const juce::String& id, int index)
{
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && ranged->getParameterID() == id)
            ranged->setValueNotifyingHost (ranged->convertTo0to1 (static_cast<float> (index)));
}

// Runs a 512-sample warm-up block, then a block of ones at `ppq`; returns that block.
static juce::AudioBuffer<float> processOnes (FakePlayHead* head, double ppq, int division = quarter)
{
    PluginProcessor plugin;
    plugin.setPlayHead (head);
    setChoice (plugin, "division", division);
    plugin.prepareToPlay (48000.0, 512);
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buffer (2, 512);
    buffer.clear();
    plugin.processBlock (buffer, midi);

    if (head != nullptr)
        head->ppq = ppq;
    for (int ch = 0; ch < 2; ++ch)
        juce::FloatVectorOperations::fill (buffer.getWritePointer (ch), 1.0f, 512);
    plugin.processBlock (buffer, midi);
    return buffer;
}

TEST_CASE ("dsp_01_processor_reads_play_head_and_division")
{
    FakePlayHead head;
    CHECK (processOnes (&head, 0.5).getSample (0, 0) == Catch::Approx (1.0f));
    CHECK (processOnes (&head, 0.5, half).getSample (0, 0) == Catch::Approx (0.5f));
    CHECK (processOnes (&head, 0.5, half).getSample (1, 0) == Catch::Approx (0.5f));
    CHECK (processOnes (&head, 0.0).getSample (0, 0) == Catch::Approx (0.0f).margin (1.0e-6));
}

TEST_CASE ("dsp_01_processor_passes_audio_without_usable_transport")
{
    FakePlayHead stopped;
    stopped.playing = false;
    FakePlayHead noPosition;
    noPosition.present = false;
    for (auto* head : { &stopped, &noPosition, static_cast<FakePlayHead*> (nullptr) })
    {
        const auto out = processOnes (head, 0.0);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                CHECK (out.getSample (ch, i) == Catch::Approx (1.0f));
    }
}
