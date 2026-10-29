#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
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
    CHECK (p->getNormalisableRange().start == Catch::Approx (0.0f));
    CHECK (p->getNormalisableRange().end == Catch::Approx (100.0f));
    CHECK (p->convertFrom0to1 (p->getDefaultValue()) == Catch::Approx (100.0f));
}

TEST_CASE ("stat_01_parameter_ids_have_version_1")
{
    PluginProcessor plugin;
    for (auto id : { "depth", "mix", "bypass" })
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

TEST_CASE ("dsp_04_bypass_is_the_host_bypass_parameter_default_off")
{
    PluginProcessor plugin;
    auto* bypass = plugin.getBypassParameter();
    REQUIRE (bypass != nullptr);
    CHECK (bypass == findParameter (plugin, "bypass"));
    CHECK (bypass->getValue() < 0.5f);
    CHECK (plugin.getLatencySamples() == 0);
}

TEST_CASE ("dsp_09_depth_and_mix_read_and_parse_whole_percent")
{
    PluginProcessor plugin;
    for (auto id : { "depth", "mix" })
    {
        auto* p = findParameter (plugin, id);
        REQUIRE (p != nullptr);
        CHECK (p->getLabel().isEmpty());

        const auto text = [p] (float percent) { return p->getText (p->convertTo0to1 (percent), 1024); };
        CHECK (text (30.4f) == "30 %");
        CHECK (text (99.6f) == "100 %");
        CHECK (text (0.0f) == "0 %");

        const auto parse = [p] (const char* t) { return p->convertFrom0to1 (p->getValueForText (t)); };
        CHECK (parse ("30") == Catch::Approx (30.0f));
        CHECK (parse ("30 %") == Catch::Approx (30.0f));
        CHECK (parse ("30%") == Catch::Approx (30.0f));
        CHECK (parse ("30.4") == Catch::Approx (30.4f));
        CHECK (parse ("150") == Catch::Approx (100.0f));
        CHECK (parse ("-5") == Catch::Approx (0.0f));
        CHECK (parse ("abc") == Catch::Approx (100.0f));
    }
}
