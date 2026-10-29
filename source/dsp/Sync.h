#pragma once

#include <cmath>
#include <optional>

namespace dsp
{
struct Transport
{
    std::optional<double> ppq, bpm;
    bool playing = false;
};

inline constexpr int numDivisions = 13;

// Quarter notes per cycle, in the order of the `division` parameter (SYNC-02).
inline double cycleBeats (int division)
{
    constexpr double beats[numDivisions] = { 4.0, 2.0, 3.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0,
                                             0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0 };
    return beats[division < 0 ? 0 : division >= numDivisions ? numDivisions - 1 : division];
}

// Phase in cycles, derived fresh from the host's PPQ each block (SYNC-01); inactive without a usable transport (SYNC-05).
class Sync
{
public:
    void prepare (double sampleRate) { rate = sampleRate; }

    void setBlock (const Transport& t, int division)
    {
        active = false;
        start = inc = 0.0;
        if (! (rate > 0.0) || ! t.playing || ! t.ppq || ! t.bpm || ! std::isfinite (*t.ppq) || ! std::isfinite (*t.bpm)
            || *t.bpm < 1.0)
            return;
        const auto beats = cycleBeats (division);
        start = *t.ppq / beats;
        inc = *t.bpm / (60.0 * rate * beats);
        active = true;
    }

    bool isActive() const { return active; }

    double phase (int sample) const
    {
        if (! active)
            return 0.0;
        const auto x = start + sample * inc;
        const auto p = x - std::floor (x);
        return p >= 1.0 ? 0.0 : p;
    }

private:
    double rate = 0.0, start = 0.0, inc = 0.0;
    bool active = false;
};
} // namespace dsp
