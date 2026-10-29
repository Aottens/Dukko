#include <dsp/Curve.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cmath>
#include <numbers>
#include <thread>

using dsp::Curve;

static Curve flat (float y) { return Curve { { { 0.0f, y, 0.0f } } }; }

// Segment shape of CURV-01 for tension t.
static double shape (double t, double u) { return (std::exp (10.0 * t * u) - 1.0) / (std::exp (10.0 * t) - 1.0); }

TEST_CASE ("curv_01_no_points_is_zero_one_point_is_flat")
{
    CHECK (Curve {}.value (0.3) == 0.0f);
    CHECK (flat (0.7f).value (0.0) == Catch::Approx (0.7f));
    CHECK (flat (0.7f).value (0.9) == Catch::Approx (0.7f));
}

TEST_CASE ("curv_01_points_connect_and_wrap")
{
    for (const auto& curve : { Curve { { { 0.25f, 1.0f, 0.0f }, { 0.75f, 0.0f, 0.0f } } },
                               Curve { { { 0.75f, 0.0f, 0.0f }, { 0.25f, 1.0f, 0.0f } } } }) // unsorted
    {
        CHECK (curve.value (0.25) == Catch::Approx (1.0f));
        CHECK (curve.value (0.75) == Catch::Approx (0.0f).margin (1.0e-6));
        CHECK (curve.value (0.5) == Catch::Approx (0.5f));  // tension 0: mean of the ends
        CHECK (curve.value (0.0) == Catch::Approx (0.5f));  // on the wrap segment 0.75 -> 1.25
        CHECK (curve.value (0.95) == Catch::Approx (0.4f));
        CHECK (curve.value (1.25) == Catch::Approx (1.0f)); // phase wraps into [0, 1)
        CHECK (curve.value (-0.75) == Catch::Approx (1.0f));
    }
}

TEST_CASE ("curv_01_tension_shapes_the_segment")
{
    const Curve slow { { { 0.0f, 0.0f, 0.5f }, { 0.5f, 1.0f, 0.0f } } };
    const Curve fast { { { 0.0f, 0.0f, -0.5f }, { 0.5f, 1.0f, 0.0f } } };
    CHECK (slow.value (0.25) == Catch::Approx (shape (0.5, 0.5)));
    CHECK (fast.value (0.25) == Catch::Approx (1.0 - shape (0.5, 0.5))); // -t mirrors t
    CHECK (slow.value (0.1) == Catch::Approx (shape (0.5, 0.2)));
    CHECK (fast.value (0.1) == Catch::Approx (1.0 - shape (0.5, 0.8)));
}

TEST_CASE ("curv_01_out_of_range_values_clamp")
{
    CHECK (flat (2.0f).value (0.5) == Catch::Approx (1.0f));
    CHECK (flat (-1.0f).value (0.5) == 0.0f);
    const Curve steep { { { 0.0f, 0.0f, 3.0f }, { 0.5f, 1.0f, 0.0f } } };
    CHECK (steep.value (0.25) == Catch::Approx (shape (1.0, 0.5)));
}

TEST_CASE ("curv_01_default_curve_follows_the_raised_cosine")
{
    const auto sine = Curve::sine();
    CHECK (sine.value (0.0) == Catch::Approx (1.0f));
    CHECK (sine.value (0.25) == Catch::Approx (0.5f));
    CHECK (sine.value (0.5) == Catch::Approx (0.0f).margin (1.0e-6));
    CHECK (sine.value (0.75) == Catch::Approx (0.5f));
    double worst = 0.0;
    for (int i = 0; i < 1000; ++i)
    {
        const auto p = i / 1000.0;
        worst = std::max (worst, std::abs (sine.value (p) - 0.5 * (1.0 + std::cos (2.0 * std::numbers::pi * p))));
    }
    CHECK (worst < 0.025);
}

TEST_CASE ("curv_02_snapshot_renders_1024_entries_and_interpolates")
{
    const auto sine = Curve::sine();
    dsp::CurveSnapshot snapshot;
    snapshot.render (sine);
    REQUIRE (snapshot.lut.size() == 1024);
    for (int i = 0; i < 1024; ++i)
        CHECK (snapshot.lut[(size_t) i] == sine.value (i / 1024.0));

    double worst = 0.0;
    for (int i = 0; i < 10000; ++i)
        worst = std::max (worst, (double) std::abs (snapshot.lookup (i / 10000.0) - sine.value (i / 10000.0)));
    CHECK (worst < 0.001);
    CHECK (snapshot.lookup (0.25) == Catch::Approx (0.5f));
    CHECK (snapshot.lookup (1023.5 / 1024.0) == Catch::Approx ((snapshot.lut[1023] + snapshot.lut[0]) / 2.0f)); // 1023 wraps to 0
}

TEST_CASE ("curv_02_newest_publish_wins")
{
    dsp::CurveFeed feed;
    feed.publish (flat (0.2f));
    feed.publish (flat (0.8f));
    feed.prepare (48000.0); // after prepare: newest snapshot, no crossfade
    feed.beginBlock();
    CHECK (feed.next (0.3) == Catch::Approx (0.8f));
}

TEST_CASE ("curv_02_no_torn_snapshot_across_threads")
{
    dsp::CurveFeed feed;
    feed.prepare (0.0); // no crossfade, so every value read comes from one snapshot
    std::atomic<bool> done { false };
    std::thread writer ([&] {
        for (int k = 1; k <= 10000; ++k)
            feed.publish (flat (static_cast<float> (k) / 10000.0f));
        done = true;
    });

    int torn = 0, reads = 0;
    for (bool last = false; ! last; ++reads)
    {
        last = done.load();
        feed.beginBlock();
        const auto first = feed.next (0.0);
        for (int i = 1; i < 1024; ++i)
            if (feed.next (i / 1024.0) != first)
                ++torn;
    }
    writer.join();
    CHECK (torn == 0);
    CHECK (reads > 0);
    feed.beginBlock();
    CHECK (feed.next (0.5) == 1.0f); // the last publish arrived
}

TEST_CASE ("curv_03_crossfade_over_10_ms")
{
    dsp::CurveFeed feed;
    feed.publish (flat (0.0f));
    feed.prepare (48000.0);
    feed.beginBlock();
    feed.publish (flat (1.0f));
    feed.beginBlock();

    float previous = 0.0f, value = 0.0f;
    for (int i = 0; i < 480; ++i)
    {
        if (i == 100) // published mid-fade: not taken until the fade ends
        {
            feed.publish (flat (0.5f));
            feed.publish (flat (0.25f));
            feed.beginBlock();
        }
        value = feed.next (0.3);
        CHECK (value - previous <= 1.0f / 480.0f + 1.0e-6f);
        CHECK (value >= previous);
        previous = value;
    }
    CHECK (value == 1.0f);
    CHECK (feed.next (0.3) == 1.0f);

    feed.beginBlock(); // now the newest one, 0.25, fades in
    for (int i = 0; i < 480; ++i)
    {
        value = feed.next (0.3);
        CHECK (previous - value <= 0.75f / 480.0f + 1.0e-6f);
        CHECK (value <= previous);
        previous = value;
    }
    CHECK (value == 0.25f);
}
