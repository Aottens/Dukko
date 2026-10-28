#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("build_05_plugin_is_named_dukko")
{
    PluginProcessor plugin;
    CHECK (plugin.getName() == "Dukko");
}

TEST_CASE ("build_06_passthrough_leaves_audio_untouched")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.5f);
    juce::MidiBuffer midi;
    plugin.processBlock (buffer, midi);
    CHECK (buffer.getMagnitude (0, 256) == 0.5f);
    CHECK (buffer.getRMSLevel (1, 0, 256) == 0.5f);
}
