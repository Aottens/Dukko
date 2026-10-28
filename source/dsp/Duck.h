#pragma once

#include <cmath>

namespace dsp
{
// Gain = 1 - mix * depth * curve, with depth and mix ramped linearly over 20 ms.
class Duck
{
public:
    void setTargets (float depth, float mix)
    {
        retarget (depthRamp, depth);
        retarget (mixRamp, mix);
    }

    void prepare (double sampleRate)
    {
        rampSamples = static_cast<int> (std::lround (0.02 * sampleRate));
        depthRamp.snap();
        mixRamp.snap();
    }

    float next (float curve)
    {
        return 1.0f - mixRamp.advance() * depthRamp.advance() * curve;
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

    void retarget (Ramp& r, float value) const
    {
        if (! (value < r.target || value > r.target)) // same target: keep the running ramp
            return;
        r.start = r.current;
        r.target = value;
        r.total = r.remaining = rampSamples;
        if (rampSamples <= 0)
            r.snap();
    }

    Ramp depthRamp, mixRamp;
    int rampSamples = 0;
};
} // namespace dsp
