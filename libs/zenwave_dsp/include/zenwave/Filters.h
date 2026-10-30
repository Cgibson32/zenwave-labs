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




// Variable-slope cut filter: 6 / 12 / 24 / 48 dB per octave.
// 6 dB uses a one-pole; 12/24/48 cascade TPT SVFs with Butterworth Q values so the
// passband stays flat and the knee is clean at any slope.
class CutFilter
{
public:
    enum class Type { Lowcut, Highcut };
    enum class Slope { dB6 = 0, dB12 = 1, dB24 = 2, dB48 = 3 };

    void setSampleRate(double sr) noexcept
    {
        sampleRate = sr;
        onePole.setSampleRate(sr);
        for (auto& s : stages) s.setSampleRate(sr);
        update();
    }
    void setType(Type t) noexcept { type = t; update(); }
    void setSlope(Slope s) noexcept { slope = s; update(); }
    void setCutoff(float hz) noexcept { cutoff = clampf(hz, 10.0f, static_cast<float>(sampleRate * 0.45)); update(); }
    void reset() noexcept { onePole.reset(); for (auto& s : stages) s.reset(); }

    float process(float x) noexcept
    {
        if (slope == Slope::dB6) return onePole.process(x);
        for (int i = 0; i < numStages; ++i) x = stages[i].process(x);
        return x;
    }

private:
    void update() noexcept
    {
        if (type == Type::Lowcut) onePole.setHighpass(cutoff); else onePole.setLowpass(cutoff);
        // Butterworth Q per stage for 2nd, 4th and 8th order
        static constexpr float q2[1] = { 0.7071f };
        static constexpr float q4[2] = { 0.5412f, 1.3066f };
        static constexpr float q8[4] = { 0.5098f, 0.6013f, 0.9000f, 2.5629f };
        const float* qs = q2; numStages = 1;
        if (slope == Slope::dB24) { qs = q4; numStages = 2; }
        if (slope == Slope::dB48) { qs = q8; numStages = 4; }
        for (int i = 0; i < numStages; ++i)
        {
            stages[i].setMode(type == Type::Lowcut ? SVF::Mode::Highpass : SVF::Mode::Lowpass);
            stages[i].setResonance(qs[i]);
            stages[i].setCutoff(cutoff);
        }
    }

    double sampleRate = 44100.0;
    float cutoff = 100.0f;
    Type type = Type::Lowcut;
    Slope slope = Slope::dB12;
    int numStages = 1;
    OnePole onePole;
    SVF stages[4];
};

// Tilt EQ: ±6 dB pivoting around 1 kHz. Negative = darker, positive = brighter.
class TiltEQ
{
public:
    void setSampleRate(double sr) noexcept { lp.setSampleRate(sr); lp.setLowpass(1000.0f); }
    void setTilt(float t) noexcept { tilt = clampf(t, -1.0f, 1.0f); }
    void reset() noexcept { lp.reset(); }
    float process(float x) noexcept
    {
        const float low = lp.process(x), high = x - low;
        const float gLow = dbToGain(-tilt * 6.0f), gHigh = dbToGain(tilt * 6.0f);
        return low * gLow + high * gHigh;
    }
private:
    OnePole lp; float tilt = 0.0f;
};

// 2x oversampler: 2-path polyphase half-band made of first-order allpass sections
// (the classic "hiir" structure). ~ -70 dB alias rejection, low latency, cheap.
class Oversampler2x
{
public:
    void reset() noexcept { for (auto* a : { &uA0, &uA1, &uB0, &uB1, &dA0, &dA1, &dB0, &dB1 }) a->reset(); }

    // One input sample at base rate → two samples at 2x rate
    void upsample(float x, float& y0, float& y1) noexcept
    {
        y0 = uA1.process(uA0.process(x));
        y1 = uB1.process(uB0.process(x));
    }
    // Two samples at 2x rate → one sample at base rate
    float downsample(float y0, float y1) noexcept
    {
        const float a = dA1.process(dA0.process(y0));
        const float b = dB1.process(dB0.process(y1));
        return 0.5f * (a + b);
    }

private:
    struct AP
    {
        float c, x1 = 0.0f, y1 = 0.0f;
        explicit AP(float coef) : c(coef) {}
        void reset() noexcept { x1 = y1 = 0.0f; }
        float process(float x) noexcept { const float y = c * (x - y1) + x1; x1 = x; y1 = y; return y; }
    };
    // 4 coefficients, alternately assigned to the two paths
    // hiir 4-coefficient set: ~69 dB attenuation, transition band 0.1 × fs
    AP uA0 { 0.041893991997656171f }, uA1 { 0.39056077292116603f }, uB0 { 0.16890348243995201f }, uB1 { 0.74389574826847926f };
    AP dA0 { 0.041893991997656171f }, dA1 { 0.39056077292116603f }, dB0 { 0.16890348243995201f }, dB1 { 0.74389574826847926f };
};

// 4x oversampler: two nested 2x stages. Use for aggressive saturation / clipping.
class Oversampler4x
{
public:
    void reset() noexcept { s1.reset(); s2.reset(); }
    // 1 sample → 4 samples. The second stage runs on the interleaved 2x stream.
    void upsample(float x, float* y4) noexcept
    {
        float a, b; s1.upsample(x, a, b);
        s2.upsample(a, y4[0], y4[1]);
        s2.upsample(b, y4[2], y4[3]);
    }
    float downsample(const float* y4) noexcept
    {
        const float a = s2.downsample(y4[0], y4[1]);
        const float b = s2.downsample(y4[2], y4[3]);
        return s1.downsample(a, b);
    }
private:
    Oversampler2x s1, s2;
};

} // namespace zw
