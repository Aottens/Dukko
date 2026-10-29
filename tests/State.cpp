#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

static juce::RangedAudioParameter& param (juce::AudioProcessor& plugin, const juce::String& id)
{
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && ranged->getParameterID() == id)
            return *ranged;
    FAIL ("no parameter " << id);
    throw;
}

static float get (juce::AudioProcessor& plugin, const juce::String& id)
{
    auto& p = param (plugin, id);
    return p.convertFrom0to1 (p.getValue());
}

static void set (juce::AudioProcessor& plugin, const juce::String& id, float value)
{
    auto& p = param (plugin, id);
    p.setValueNotifyingHost (p.convertTo0to1 (value));
}

static juce::MemoryBlock save (juce::AudioProcessor& plugin)
{
    juce::MemoryBlock block;
    plugin.getStateInformation (block);
    return block;
}

static void load (juce::AudioProcessor& plugin, const juce::MemoryBlock& block)
{
    plugin.setStateInformation (block.getData(), static_cast<int> (block.getSize()));
}

static juce::MemoryBlock toBlock (const juce::ValueTree& tree)
{
    juce::MemoryBlock block;
    juce::MemoryOutputStream stream (block, false);
    tree.writeToStream (stream);
    return block;
}

static juce::ValueTree toTree (const juce::MemoryBlock& block)
{
    return juce::ValueTree::readFromData (block.getData(), block.getSize());
}

static juce::ValueTree paramTree (const juce::String& id, float value)
{
    return juce::ValueTree ("PARAM", { { "id", id }, { "value", value } });
}

// The pre-VS1-03 format: no version attribute.
static juce::ValueTree legacyState (float depth, float mix, float bypass)
{
    juce::ValueTree tree ("DukkoState");
    tree.appendChild (paramTree ("depth", depth), nullptr);
    tree.appendChild (paramTree ("mix", mix), nullptr);
    tree.appendChild (paramTree ("bypass", bypass), nullptr);
    return tree;
}

TEST_CASE ("stat_02_saved_state_is_versioned_with_every_parameter")
{
    PluginProcessor plugin;
    set (plugin, "depth", 30.0f);
    set (plugin, "mix", 70.0f);
    set (plugin, "bypass", 1.0f);

    const auto block = save (plugin);
    const auto tree = toTree (block);
    REQUIRE (tree.hasType ("DukkoState"));
    CHECK (static_cast<int> (tree.getProperty ("version", 0)) == 1);
    CHECK (tree.getNumChildren() == 3);
    for (auto [id, value] : { std::pair { "depth", 30.0f }, std::pair { "mix", 70.0f }, std::pair { "bypass", 1.0f } })
    {
        const auto child = tree.getChildWithProperty ("id", id);
        REQUIRE (child.isValid());
        CHECK (static_cast<float> (child.getProperty ("value")) == Catch::Approx (value));
    }

    PluginProcessor fresh;
    load (fresh, block);
    CHECK (get (fresh, "depth") == Catch::Approx (30.0f));
    CHECK (get (fresh, "mix") == Catch::Approx (70.0f));
    CHECK (get (fresh, "bypass") == Catch::Approx (1.0f));
}

TEST_CASE ("stat_05_load_is_deterministic_and_idempotent")
{
    juce::ValueTree data ("DukkoState", { { "version", 1 }, { "stranger", "x" } });
    data.appendChild (paramTree ("depth", 250.0f), nullptr);                       // out of range
    data.appendChild (juce::ValueTree ("PARAM", { { "id", "bypass" } }), nullptr); // no value
    data.appendChild (paramTree ("future", 5.0f), nullptr);                        // unknown id
    data.appendChild (juce::ValueTree ("Unknown"), nullptr);                       // unknown child
    // mix is missing
    const auto block = toBlock (data);

    PluginProcessor a;
    PluginProcessor b;
    set (b, "depth", 10.0f);
    set (b, "mix", 30.0f);
    set (b, "bypass", 1.0f);
    load (a, block);
    load (b, block);

    for (auto* plugin : { &a, &b })
    {
        CHECK (get (*plugin, "depth") == Catch::Approx (100.0f));
        CHECK (get (*plugin, "mix") == Catch::Approx (100.0f));
        CHECK (get (*plugin, "bypass") == Catch::Approx (0.0f));
    }
    CHECK (save (a) == save (b));

    const auto saved = toTree (save (a));
    CHECK (saved.getNumChildren() == 3);
    CHECK (! saved.hasProperty ("stranger"));
    CHECK (! saved.getChildWithProperty ("id", "future").isValid());

    const auto once = save (a);
    load (a, block);
    CHECK (save (a) == once);
}

TEST_CASE ("stat_05_data_that_is_not_dukko_state_changes_nothing")
{
    PluginProcessor plugin;
    set (plugin, "depth", 40.0f);
    const auto before = save (plugin);

    plugin.setStateInformation (nullptr, 0);
    const char garbage[] = "not a value tree";
    plugin.setStateInformation (garbage, sizeof (garbage));
    juce::ValueTree other ("OtherState", { { "version", 1 } });
    other.appendChild (paramTree ("depth", 10.0f), nullptr);
    load (plugin, toBlock (other));

    CHECK (save (plugin) == before);
}

TEST_CASE ("stat_06_unversioned_state_loads_and_saves_as_version_1")
{
    PluginProcessor plugin;
    load (plugin, toBlock (legacyState (25.0f, 60.0f, 1.0f)));
    CHECK (get (plugin, "depth") == Catch::Approx (25.0f));
    CHECK (get (plugin, "mix") == Catch::Approx (60.0f));
    CHECK (get (plugin, "bypass") == Catch::Approx (1.0f));
    CHECK (static_cast<int> (toTree (save (plugin)).getProperty ("version", 0)) == 1);
}

TEST_CASE ("stat_06_newer_version_loads_best_effort_and_saves_as_version_1")
{
    auto data = legacyState (25.0f, 60.0f, 0.0f);
    data.setProperty ("version", 99, nullptr);
    data.appendChild (juce::ValueTree ("Curve"), nullptr);

    PluginProcessor plugin;
    load (plugin, toBlock (data));
    CHECK (get (plugin, "depth") == Catch::Approx (25.0f));
    CHECK (get (plugin, "mix") == Catch::Approx (60.0f));
    const auto saved = toTree (save (plugin));
    CHECK (static_cast<int> (saved.getProperty ("version", 0)) == 1);
    CHECK (! saved.getChildWithName ("Curve").isValid());
}
