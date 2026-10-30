#pragma once
// ZenWave DSP — fractional delay line with Hermite interpolation.
// Modulation-safe (used for tape wow/flutter) and allocation-free in the audio thread.

#include "Utils.h"
#include <vector>

namespace zw {

class DelayLine
{
public:
    void prepare(double sampleRate, float maxDelaySeconds) noexcept
    {
        const size_t needed = static_cast<size_t>(sampleRate * maxDelaySeconds) + 8;
        size_t n = 1;
        while (n < needed) n <<= 1;       // power of two for cheap wrap
        buffer.assign(n, 0.0f);
        mask = n - 1;
        writePos = 0;
    }

    void reset() noexcept { std::fill(buffer.begin(), buffer.end(), 0.0f); writePos = 0; }

    size_t maxDelaySamples() const noexcept { return buffer.size() - 4; }

    void write(float x) noexcept
    {
        buffer[writePos] = undenormal(x);
        writePos = (writePos + 1) & mask;
    }

    // Read `delaySamples` behind the write head (fractional, >= 1).
    float read(float delaySamples) const noexcept
    {
        delaySamples = clampf(delaySamples, 1.0f, static_cast<float>(maxDelaySamples()));
        const float  rp   = static_cast<float>(writePos) - delaySamples;
        const int    i    = static_cast<int>(std::floor(rp));
        const float  frac = rp - static_cast<float>(i);

        const float xm1 = at(i - 1), x0 = at(i), x1 = at(i + 1), x2 = at(i + 2);

        // 4-point, 3rd-order Hermite
        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    // Integer read (no interpolation) for static allpass/comb structures.
    float readInt(int delaySamples) const noexcept
    {
        return at(static_cast<int>(writePos) - delaySamples);
    }

private:
    float at(int idx) const noexcept
    {
        return buffer[static_cast<size_t>(idx) & mask];
    }

    std::vector<float> buffer;
    size_t mask = 0;
    size_t writePos = 0;
};

} // namespace zw
