#include <dsp/Sync.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>

static constexpr int quarter = 4, dottedQuarter = 5, quarterTriplet = 6; // division indices (SYNC-02)

static dsp::Transport playingAt (double ppq, double bpm = 120.0) { return { ppq, bpm, true }; }

static double phaseAt (double ppq, int sample, int division = quarter)
{
    dsp::Sync sync;
    sync.prepare (48000.0);
    sync.setBlock (playingAt (ppq), division);
    REQUIRE (sync.isActive());
    return sync.phase (sample);
}

TEST_CASE ("sync_01_phase_follows_host_ppq")
{
    CHECK (phaseAt (0.0, 0) == Catch::Approx (0.0));
    CHECK (phaseAt (0.0, 12000) == Catch::Approx (0.5));
    CHECK (phaseAt (1.25, 0) == Catch::Approx (0.25));
    CHECK (phaseAt (-0.25, 0) == Catch::Approx (0.75));
    const auto tiny = phaseAt (-1.0e-17, 0);
    CHECK (tiny >= 0.0);
    CHECK (tiny < 1.0);
}

TEST_CASE ("sync_01_phase_is_rederived_per_block_never_accumulated")
{
    dsp::Sync looped, fresh;
    looped.prepare (48000.0);
    fresh.prepare (48000.0);

    looped.setBlock (playingAt (7.9), quarter);
    looped.setBlock (playingAt (0.0), quarter); // loop back
    fresh.setBlock (playingAt (0.0), quarter);
    CHECK (looped.phase (0) == Catch::Approx (0.0));
    for (int i : { 0, 1, 255, 12000 })
        CHECK (looped.phase (i) == Catch::Approx (fresh.phase (i)).margin (0.0));
}

TEST_CASE ("sync_01_no_drift_over_five_minutes")
{
    dsp::Sync sync;
    sync.prepare (48000.0);
    const auto blocks = 5 * 60 * 48000 / 256;
    for (long n = 0; n < blocks; ++n)
    {
        const auto ppq = static_cast<double> (n * 256) * 120.0 / (60.0 * 48000.0);
        sync.setBlock (playingAt (ppq), quarter);
        const auto expected = ppq - std::floor (ppq);
        if (std::abs (sync.phase (0) - expected) > 1.0e-9)
        {
            FAIL ("block " << n << " phase " << sync.phase (0) << " expected " << expected);
        }
    }
}

TEST_CASE ("sync_02_division_sets_cycle_length_in_quarter_notes")
{
    const double expected[] = { 4.0, 2.0, 3.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0,
                                0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0 };
    static_assert (sizeof (expected) / sizeof (expected[0]) == dsp::numDivisions);
    for (int d = 0; d < dsp::numDivisions; ++d)
        CHECK (dsp::cycleBeats (d) == Catch::Approx (expected[d]));

    CHECK (phaseAt (0.0, 8000, quarterTriplet) == Catch::Approx (0.5));
    CHECK (phaseAt (0.0, 12000, quarter) == Catch::Approx (0.5));
    CHECK (phaseAt (0.0, 18000, dottedQuarter) == Catch::Approx (0.5));
}

TEST_CASE ("sync_05_unusable_transport_is_inactive_with_phase_0")
{
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    const dsp::Transport bad[] = {
        {},                          // no play head or no position
        { std::nullopt, 120.0, true }, // ppq missing
        { nan, 120.0, true },
        { inf, 120.0, true },
        { 1.0, std::nullopt, true }, // bpm missing
        { 1.0, nan, true },
        { 1.0, inf, true },
        { 1.0, 0.99, true },
        { 1.0, 0.0, true },
        { 1.0, -120.0, true },
        { 1.0, 120.0, false }, // stopped
    };
    for (const auto& t : bad)
    {
        dsp::Sync sync;
        sync.prepare (48000.0);
        sync.setBlock (playingAt (0.5), quarter); // was active the block before
        sync.setBlock (t, quarter);
        CHECK (! sync.isActive());
        for (int i : { 0, 100, 255 })
            CHECK (sync.phase (i) == Catch::Approx (0.0).margin (0.0));
    }

    for (double rate : { 0.0, -48000.0 })
    {
        dsp::Sync sync;
        sync.prepare (rate);
        sync.setBlock (playingAt (0.5), quarter);
        CHECK (! sync.isActive());
        CHECK (sync.phase (10) == Catch::Approx (0.0).margin (0.0));
    }

    dsp::Sync unprepared;
    unprepared.setBlock (playingAt (0.5), quarter);
    CHECK (! unprepared.isActive());

    dsp::Sync slow;
    slow.prepare (48000.0);
    slow.setBlock (playingAt (0.5, 1.0), quarter); // exactly 1 bpm is still valid
    CHECK (slow.isActive());
    CHECK (std::isfinite (slow.phase (255)));
    CHECK (slow.phase (0) == Catch::Approx (0.5));
}
