#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace dsp
{
struct Point
{
    float x, y, tension;
};

// Periodic piecewise curve in [0, 1]; message-thread code, may allocate (CURV-01).
struct Curve
{
    std::vector<Point> points;

    float value (double phase) const
    {
        if (points.empty())
            return 0.0f;
        auto pts = points;
        for (auto& q : pts)
        {
            q.x = std::clamp (q.x, 0.0f, std::nextafter (1.0f, 0.0f));
            q.y = std::clamp (q.y, 0.0f, 1.0f);
            q.tension = std::clamp (q.tension, -1.0f, 1.0f);
        }
        if (pts.size() == 1)
            return pts[0].y;
        std::stable_sort (pts.begin(), pts.end(), [] (const Point& a, const Point& b) { return a.x < b.x; });

        auto p = phase - std::floor (phase);
        if (p >= 1.0)
            p = 0.0;
        size_t i = pts.size() - 1; // before the first point: wrap segment
        for (size_t k = 0; k < pts.size(); ++k)
            if (pts[k].x <= p)
                i = k;
        const auto& a = pts[i];
        const auto& b = pts[(i + 1) % pts.size()];
        const double x0 = a.x, x1 = i + 1 == pts.size() ? b.x + 1.0 : b.x;
        const auto pp = p < x0 ? p + 1.0 : p;
        const auto u = x1 > x0 ? (pp - x0) / (x1 - x0) : 0.0;
        const double t = a.tension;
        const auto s = std::abs (t) < 1.0e-6 ? u : std::expm1 (10.0 * t * u) / std::expm1 (10.0 * t);
        return static_cast<float> (a.y + (b.y - a.y) * s);
    }

    static Curve sine() { return { { { 0.0f, 1.0f, 0.17f }, { 0.25f, 0.5f, -0.17f }, { 0.5f, 0.0f, 0.17f }, { 0.75f, 0.5f, -0.17f } } }; }
};

// The curve as 1024 values, read with linear interpolation (CURV-02).
struct CurveSnapshot
{
    std::array<float, 1024> lut {};

    void render (const Curve& c)
    {
        for (size_t i = 0; i < lut.size(); ++i)
            lut[i] = c.value (static_cast<double> (i) / 1024.0);
    }

    float lookup (double phase) const
    {
        const auto pos = (phase - std::floor (phase)) * 1024.0;
        const auto i = static_cast<size_t> (pos) & 1023;
        const auto a = lut[i], b = lut[(i + 1) & 1023];
        return a + (b - a) * static_cast<float> (pos - std::floor (pos));
    }
};

// Triple buffer from the message thread to the audio thread, with a 10 ms crossfade on each new snapshot (CURV-02, CURV-03).
class CurveFeed
{
public:
    static_assert (std::atomic<int>::is_always_lock_free);

    void publish (const Curve& c) // one thread only
    {
        slot (writeSlot).render (c);
        writeSlot = shared.exchange (writeSlot | newFlag) & 3;
    }

    void prepare (double sampleRate)
    {
        fadeLength = static_cast<int> (std::lround (0.01 * sampleRate));
        fadePos = fadeLength;
        if (shared.load() & newFlag)
            readSlot = shared.exchange (readSlot) & 3;
    }

    void beginBlock()
    {
        if (fadePos < fadeLength || ! (shared.load() & newFlag))
            return;
        old = slot (readSlot);
        readSlot = shared.exchange (readSlot) & 3;
        fadePos = 0;
        if (fadeLength <= 0)
            fadePos = fadeLength;
    }

    float next (double phase)
    {
        const auto v = slot (readSlot).lookup (phase);
        if (fadePos >= fadeLength)
            return v;
        const auto f = static_cast<float> (++fadePos) / static_cast<float> (fadeLength);
        return fadePos == fadeLength ? v : old.lookup (phase) * (1.0f - f) + v * f;
    }

private:
    static constexpr int newFlag = 4;
    CurveSnapshot& slot (int i) { return slots[static_cast<size_t> (i)]; }
    std::array<CurveSnapshot, 3> slots;
    CurveSnapshot old;
    std::atomic<int> shared { 2 };
    int writeSlot = 0, readSlot = 1;
    int fadeLength = 0, fadePos = 0;
};
} // namespace dsp
