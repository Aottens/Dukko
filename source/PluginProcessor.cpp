#include "PluginProcessor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto [id, name] : { std::pair { "depth", "Depth" }, std::pair { "mix", "Mix" } })
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; })
                .withValueFromStringFunction ([] (const juce::String& text) {
                    const auto t = text.trimStart();
                    return t.initialSectionContainingOnly ("+-.0123456789").containsAnyOf ("0123456789")
                               ? juce::jlimit (0.0f, 100.0f, t.getFloatValue()) : 100.0f;
                })));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "division", 1 }, "Division",
        juce::StringArray { "1/1", "1/2", "1/2.", "1/2T", "1/4", "1/4.", "1/4T", "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T" }, 4));
    return layout;
}

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DukkoState", createLayout())
{
    depthParam = apvts.getRawParameterValue ("depth");
    mixParam = apvts.getRawParameterValue ("mix");
    bypassParam = apvts.getRawParameterValue ("bypass");
    divisionParam = apvts.getRawParameterValue ("division");
    bypassParameter = apvts.getParameter ("bypass");
    apvts.state.setProperty ("version", stateVersion, nullptr);
}

void PluginProcessor::updateTargets()
{
    duck.setTargets (depthParam->load() / 100.0f, mixParam->load() / 100.0f);
    duck.setBypassed (bypassParam->load() >= 0.5f);
}

void PluginProcessor::prepareToPlay (double sampleRate, int)
{
    updateTargets();
    duck.prepare (sampleRate);
    sync.prepare (sampleRate);
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateTargets();

    dsp::Transport transport;
    if (auto* playHead = getPlayHead())
        if (const auto position = playHead->getPosition())
        {
            if (const auto ppq = position->getPpqPosition())
                transport.ppq = *ppq;
            if (const auto bpm = position->getBpm())
                transport.bpm = *bpm;
            transport.playing = position->getIsPlaying();
        }
    sync.setBlock (transport, juce::roundToInt (divisionParam->load()));
    duck.setActive (sync.isActive());

    const auto numChannels = buffer.getNumChannels();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto g = duck.next (dsp::sineCurve (sync.phase (i)));
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == layouts.getMainInputChannelSet();
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, false);
    apvts.copyState().writeToStream (stream);
}

// One step per version; a newer version than ours is loaded best-effort.
static void migrate (juce::ValueTree& tree, int current)
{
    for (int version = tree.getProperty ("version", 0); version < current; ++version)
    {
        if (version == 0)
            continue; // 0 -> 1: same layout, the version attribute was just missing
    }
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, static_cast<size_t> (sizeInBytes));
    if (! tree.isValid() || ! tree.hasType ("DukkoState"))
        return;

    migrate (tree, stateVersion);
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const auto value = tree.getChildWithProperty ("id", ranged->getParameterID()).getProperty ("value");
            ranged->setValueNotifyingHost (value.isVoid() ? ranged->getDefaultValue()
                                                          : ranged->convertTo0to1 (static_cast<float> (value)));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
