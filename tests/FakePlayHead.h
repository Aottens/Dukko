#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// A host play head the tests set directly; `present = false` reports no position.
struct FakePlayHead : juce::AudioPlayHead
{
    juce::Optional<PositionInfo> getPosition() const override
    {
        if (! present)
            return {};
        PositionInfo info;
        info.setPpqPosition (ppq);
        info.setBpm (bpm);
        info.setIsPlaying (playing);
        return info;
    }

    bool present = true, playing = true;
    double ppq = 0.0, bpm = 120.0;
};
