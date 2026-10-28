#include <dsp/Duck.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static float settledGain (float depth, float mix, float curve)
{
    dsp::Duck duck;
    duck.setTargets (depth, mix);
    duck.prepare (48000.0);
    return duck.next (curve);
}

TEST_CASE ("dsp_02_depth_sets_how_deep_the_duck_goes")
{
    CHECK (settledGain (1.0f, 1.0f, 1.0f) == Catch::Approx (0.0f));
    CHECK (settledGain (0.5f, 1.0f, 1.0f) == Catch::Approx (0.5f));
    CHECK (settledGain (1.0f, 1.0f, 0.0f) == Catch::Approx (1.0f));
}

TEST_CASE ("dsp_03_mix_blends_dry_separately_from_depth")
{
    CHECK (settledGain (1.0f, 0.0f, 1.0f) == Catch::Approx (1.0f));
    CHECK (settledGain (0.3f, 0.0f, 0.7f) == Catch::Approx (1.0f));
    CHECK (settledGain (1.0f, 0.5f, 1.0f) == Catch::Approx (0.5f));
}

TEST_CASE ("dsp_05_depth_and_mix_ramp_linearly_over_20_ms")
{
    for (int which = 0; which < 2; ++which)
    {
        dsp::Duck duck;
        duck.setTargets (which == 0 ? 0.0f : 1.0f, which == 0 ? 1.0f : 0.0f);
        duck.prepare (48000.0);
        float previous = duck.next (1.0f);
        CHECK (previous == Catch::Approx (1.0f)); // no ramp right after prepare

        duck.setTargets (1.0f, 1.0f); // step depth (which 0) or mix (which 1) from 0 to 100 %
        float gain = previous;
        for (int i = 0; i < 960; ++i)
        {
            gain = duck.next (1.0f);
            CHECK (std::abs (gain - previous) <= 1.0f / 960.0f + 1.0e-6f);
            previous = gain;
        }
        CHECK (gain == Catch::Approx (0.0f).margin (1.0e-5));
        CHECK (duck.next (1.0f) == Catch::Approx (0.0f).margin (1.0e-5));
    }
}
