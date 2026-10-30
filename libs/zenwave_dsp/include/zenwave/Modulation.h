#pragma once
// ZenWave DSP — LFOs and a tape "wow & flutter" generator.

#include "Utils.h"

namespace zw {

class LFO
{
public:
    enum class Shape { Sine, Triangle, RandomSmooth };

    void setSampleRate(double sr) noexcept { sampleRate = sr; setRate(rateHz); }
    void setRate(float hz) noexcept { rateHz = hz; inc = hz / static_cast<float>(sampleRate); }
    void setShape(Shape s) noexcept { shape = s; }
    void reset(float phase0 = 0.0f) noexcept { phase = phase0; }

    // Returns -1..1
    float next() noexcept
    {
        float out = 0.0f;
        switch (shape)
        {
            case Shape::Sine:     out = std::sin(phase * kTwoPi); break;
            case Shape::Triangle: out = 4.0f * std::fabs(phase - 0.5f) - 1.0f; break;
            case Shape::RandomSmooth:
            {
                // Sample-and-hold target each cycle, glide toward it
                if (phase + inc >= 1.0f) target = noise.next();
                held += (target - held) * std::min(1.0f, inc * 8.0f);
                out = held;
                break;
            }
        }
        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

private:
    double sampleRate = 44100.0;
    float rateHz = 1.0f, inc = 0.0f, phase = 0.0f, held = 0.0f, target = 0.0f;
    Shape shape = Shape::Sine;
    Noise noise { 0xA5A5A5A5u };
};

// Tape transport instability: slow wow (~0.5 Hz), faster flutter (~6 Hz),
// plus a touch of random drift. Output is a delay offset in samples.
class WowFlutter
{
public:
    void setSampleRate(double sr) noexcept
    {
        sampleRate = sr;
        wow.setSampleRate(sr);     wow.setRate(0.45f);  wow.setShape(LFO::Shape::Sine);
        flutter.setSampleRate(sr); flutter.setRate(6.3f); flutter.setShape(LFO::Shape::Triangle);
        drift.setSampleRate(sr);   drift.setRate(0.12f); drift.setShape(LFO::Shape::RandomSmooth);
        wow.reset(0.0f); flutter.reset(0.31f); drift.reset(0.77f);
    }

    // amount 0..1 → up to ~4 ms of modulation
    void setAmount(float a) noexcept { amount = clampf(a, 0.0f, 1.0f); }

    float nextOffsetSamples() noexcept
    {
        const float ms = amount * (wow.next() * 2.2f + flutter.next() * 0.35f + drift.next() * 1.4f);
        return ms * 0.001f * static_cast<float>(sampleRate);
    }

private:
    double sampleRate = 44100.0;
    float amount = 0.0f;
    LFO wow, flutter, drift;
};

} // namespace zw
