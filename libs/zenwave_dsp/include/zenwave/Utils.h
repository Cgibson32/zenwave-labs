#pragma once
// ZenWave DSP — small utilities shared by every block. Header-only, no JUCE.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace zw {

constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

inline float clampf(float v, float lo, float hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }
inline int   clampi(int v, int lo, int hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerp(float a, float b, float t) noexcept { return a + (b - a) * t; }

inline float dbToGain(float dB) noexcept { return std::pow(10.0f, dB * 0.05f); }
inline float gainToDb(float g)  noexcept { return 20.0f * std::log10(std::max(g, 1.0e-9f)); }

// Fast, smooth saturation. Good enough for feedback paths; the main
// saturator uses the oversampled version in Saturator.h.
inline float fastTanh(float x) noexcept
{
    const float x2 = x * x;
    const float a  = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
    const float b  = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
    return clampf(a / b, -1.0f, 1.0f);
}

// Transparent soft ceiling: unity below ~0.7, smooth knee, never exceeds 1.0.
inline float softCeiling(float x) noexcept
{
    const float a = std::fabs(x);
    if (a <= 0.7f) return x;
    const float over = a - 0.7f;
    const float y = 0.7f + 0.3f * fastTanh(over / 0.3f);
    return x < 0.0f ? -y : y;
}

inline bool isDenormal(float v) noexcept { return std::fabs(v) < 1.0e-25f; }
inline float undenormal(float v) noexcept { return isDenormal(v) ? 0.0f : v; }

// One-pole parameter smoother (per-sample). Call setTime() after setSampleRate().
class Smoother
{
public:
    void setSampleRate(double sr) noexcept { sampleRate = sr; setTime(timeMs); }
    void setTime(float ms) noexcept
    {
        timeMs = ms;
        const double samples = std::max(1.0, sampleRate * ms * 0.001);
        coeff = static_cast<float>(std::exp(-1.0 / samples));
    }
    void reset(float v) noexcept { current = target = v; }
    void setTarget(float v) noexcept { target = v; }
    float getTarget() const noexcept { return target; }
    float next() noexcept { current = target + coeff * (current - target); return current; }
    float value() const noexcept { return current; }

private:
    double sampleRate = 44100.0;
    float  timeMs = 20.0f;
    float  coeff = 0.0f, current = 0.0f, target = 0.0f;
};

// Deterministic white noise (xorshift). Cheap, and identical across platforms.
class Noise
{
public:
    explicit Noise(uint32_t seed = 0x9E3779B9u) : state(seed ? seed : 1u) {}
    float next() noexcept
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return (static_cast<float>(state) * (1.0f / 4294967296.0f)) * 2.0f - 1.0f;
    }
private:
    uint32_t state;
};

// DC blocker (first-order highpass at ~5 Hz).
class DCBlocker
{
public:
    void setSampleRate(double sr) noexcept { R = static_cast<float>(1.0 - (kTwoPi * 5.0 / sr)); }
    void reset() noexcept { x1 = y1 = 0.0f; }
    float process(float x) noexcept
    {
        const float y = x - x1 + R * y1;
        x1 = x; y1 = undenormal(y);
        return y;
    }
private:
    float R = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

} // namespace zw
