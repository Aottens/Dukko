#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>

static juce::RangedAudioParameter* findParameter (juce::AudioProcessor& plugin, const juce::String& id)
{
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && ranged->getParameterID() == id)
            return ranged;
    return nullptr;
}

static void checkPercentParameter (const juce::String& id)
{
    PluginProcessor plugin;
    auto* p = findParameter (plugin, id);
    REQUIRE (p != nullptr);
    CHECK (p->getNormalisableRange().start == 0.0f);
    CHECK (p->getNormalisableRange().end == 100.0f);
    CHECK (p->convertFrom0to1 (p->getDefaultValue()) == 100.0f);
}

TEST_CASE ("stat_01_parameter_ids_have_version_1")
{
    PluginProcessor plugin;
    for (auto id : { "depth", "mix" })
    {
        auto* p = findParameter (plugin, id);
        REQUIRE (p != nullptr);
        CHECK (p->getVersionHint() == 1);
    }
}

TEST_CASE ("dsp_02_depth_parameter_is_0_to_100_percent_default_100")
{
    checkPercentParameter ("depth");
}

TEST_CASE ("dsp_03_mix_parameter_is_0_to_100_percent_default_100")
{
    checkPercentParameter ("mix");
}
