#pragma once
// ZenWave DSP — filters. One-pole tone shaping and a TPT state-variable filter
// (Zavalishin), which stays stable under fast modulation. Header-only, no JUCE.

#include "Utils.h"

namespace zw {

// Simple one-pole lowpass / highpass for tone shaping in feedback paths.
class OnePole
{
public:
    void setSampleRate(double sr) noexcept { sampleRate = sr; }
    void setLowpass(float hz) noexcept  { setCoeff(hz); highpass = false; }
    void setHighpass(float hz) noexcept { setCoeff(hz); highpass = true; }
    void reset() noexcept { z = 0.0f; }

    float process(float x) noexcept
    {
        z = z + a * (x - z);
        z = undenormal(z);
        return highpass ? x - z : z;
    }

private:
    void setCoeff(float hz) noexcept
    {
        hz = clampf(hz, 1.0f, static_cast<float>(sampleRate * 0.49));
        a  = 1.0f - std::exp(-kTwoPi * hz / static_cast<float>(sampleRate));
    }
    double sampleRate = 44100.0;
    float  a = 0.1f, z = 0.0f;
    bool   highpass = false;
};

// Topology-preserving-transform SVF. Outputs LP/BP/HP simultaneously.
class SVF
{
public:
    enum class Mode { Lowpass, Bandpass, Highpass, Notch };

    void setSampleRate(double sr) noexcept { sampleRate = sr; update(); }
    void setCutoff(float hz) noexcept { cutoff = clampf(hz, 10.0f, static_cast<float>(sampleRate * 0.45)); update(); }
    void setResonance(float q) noexcept { Q = clampf(q, 0.1f, 20.0f); update(); }
    void setMode(Mode m) noexcept { mode = m; }
    void reset() noexcept { ic1eq = ic2eq = 0.0f; }

    float process(float v0) noexcept
    {
        const float v3 = v0 - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = undenormal(2.0f * v1 - ic1eq);
        ic2eq = undenormal(2.0f * v2 - ic2eq);

        switch (mode)
        {
            case Mode::Lowpass:  return v2;
            case Mode::Bandpass: return v1;
            case Mode::Highpass: return v0 - k * v1 - v2;
            case Mode::Notch:    return v0 - k * v1;
        }
        return v2;
    }

private:
    void update() noexcept
    {
        const float g = std::tan(kPi * cutoff / static_cast<float>(sampleRate));
        k  = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    double sampleRate = 44100.0;
    float  cutoff = 1000.0f, Q = 0.7071f;
    float  k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float  ic1eq = 0.0f, ic2eq = 0.0f;
    Mode   mode = Mode::Lowpass;
};

} // namespace zw
