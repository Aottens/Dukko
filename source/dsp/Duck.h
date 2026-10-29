#pragma once

#include <cmath>
#include <numbers>

namespace dsp
{
// Raised cosine over one cycle: 1 at phase 0, 0 at phase 0.5 (DSP-01).
inline float sineCurve (double phase) { return static_cast<float> (0.5 * (1.0 + std::cos (2.0 * std::numbers::pi * phase))); }

// Gain = 1 - (1 - bypass) * active * mix * depth * curve; depth and mix ramp linearly over 20 ms, bypass and active over 10 ms.
class Duck
{
public:
    void setTargets (float depth, float mix)
    {
        retarget (depthRamp, depth, rampSamples);
        retarget (mixRamp, mix, rampSamples);
    }

    void setBypassed (bool bypassed) { retarget (bypassRamp, bypassed ? 1.0f : 0.0f, bypassRampSamples); }

    void setActive (bool active) { retarget (activeRamp, active ? 1.0f : 0.0f, bypassRampSamples); }

    void prepare (double sampleRate)
    {
        rampSamples = static_cast<int> (std::lround (0.02 * sampleRate));
        bypassRampSamples = static_cast<int> (std::lround (0.01 * sampleRate));
        depthRamp.snap();
        mixRamp.snap();
        bypassRamp.snap();
        activeRamp.snap();
    }

    float next (float curve)
    {
        return 1.0f - (1.0f - bypassRamp.advance()) * activeRamp.advance() * mixRamp.advance() * depthRamp.advance() * curve;
    }

private:
    struct Ramp
    {
        float current = 0.0f, target = 0.0f, start = 0.0f;
        int total = 0;
        int remaining = 0;

        void snap() { current = target; remaining = 0; }

        float advance()
        {
            if (remaining > 0)
                current = --remaining == 0 ? target : target + (start - target) * static_cast<float> (remaining) / static_cast<float> (total);
            return current;
        }
    };

    static void retarget (Ramp& r, float value, int length)
    {
        if (! (value < r.target || value > r.target)) // same target: keep the running ramp
            return;
        r.start = r.current;
        r.target = value;
        r.total = r.remaining = length;
        if (length <= 0)
            r.snap();
    }

    Ramp depthRamp, mixRamp, bypassRamp, activeRamp;
    int rampSamples = 0, bypassRampSamples = 0;
};
} // namespace dsp
