#pragma once
// ZenWave DSP — SpaceReverb v2.
// 8-line feedback delay network (Householder mixing) with:
//   - 4 modulated input allpasses (dense onset, no discrete early echoes)
//   - per-line decay gains computed from an RT60 time, so Decay means seconds, not a magic number
//   - per-line lowpass damping and a loop highpass (sub-safe)
//   - slow per-line delay modulation (smooth, chorused tail, no metallic ringing)
//   - 2 output allpasses per channel for extra density and decorrelation
//   - Freeze: input muted, loop gain pinned just below unity, soft-limited

#include "Utils.h"
#include "Filters.h"
#include "DelayLine.h"
#include "Modulation.h"
#include <array>

namespace zw {

class SpaceReverb
{
public:
    static constexpr int kLines = 8;

    void prepare(double sr) noexcept
    {
        sampleRate = sr;
        for (int i = 0; i < kLines; ++i)
        {
            lines[i].prepare(sr, 0.6f);
            damp[i].setSampleRate(sr);
            lineHP[i].setSampleRate(sr);
            lineHP[i].setHighpass(40.0f);
            lfo[i].setSampleRate(sr);
            lfo[i].setShape(LFO::Shape::Sine);
            lfo[i].reset(static_cast<float>(i) / kLines);
        }
        for (int i = 0; i < 4; ++i)
        {
            inAP[i].prepare(sr, 0.08f);
            inLfo[i].setSampleRate(sr);
            inLfo[i].setRate(0.11f + 0.07f * i);
            inLfo[i].setShape(LFO::Shape::Sine);
            inLfo[i].reset(0.25f * i);
        }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 2; ++i) outAP[c][i].prepare(sr, 0.03f);
        loopHP.setSampleRate(sr);
        loopHP.setHighpass(90.0f);
        setModRate(1.0f);
        setSize(0.6f);
        setDecaySeconds(3.0f);
        setDamping(8000.0f);
        reset();
    }

    void reset() noexcept
    {
        for (auto& l : lines) l.reset();
        for (auto& a : inAP) a.reset();
        for (auto& c : outAP) for (auto& a : c) a.reset();
        for (auto& f : damp) f.reset();
        for (auto& f : lineHP) f.reset();
        loopHP.reset();
    }

    // size 0..1 → line lengths from ~0.5x to ~2x of the base primes (small room → cathedral → void)
    void setSize(float s) noexcept
    {
        size = clampf(s, 0.0f, 1.0f);
        const float scale = 0.5f + size * 1.5f;
        for (int i = 0; i < kLines; ++i)
            baseDelay[i] = kBaseMs[i] * 0.001f * static_cast<float>(sampleRate) * scale;
        updateGains();
    }

    // Decay as RT60 in seconds (0.2 .. 60). Use setDecay(0..1) for a musical curve.
    void setDecaySeconds(float seconds) noexcept { rt60 = clampf(seconds, 0.2f, 60.0f); updateGains(); }
    void setDecay(float d) noexcept { setDecaySeconds(0.4f * std::pow(60.0f, clampf(d, 0.0f, 1.0f))); } // 0.4 s .. 24 s

    void setDamping(float hz) noexcept { for (auto& f : damp) f.setLowpass(clampf(hz, 500.0f, 20000.0f)); }
    void setLoopHighpass(float hz) noexcept { loopHP.setHighpass(hz); for (auto& f : lineHP) f.setHighpass(hz * 0.5f); }
    void setDiffusion(float g) noexcept { diffusion = 0.45f + clampf(g, 0.0f, 1.0f) * 0.35f; }
    void setModDepth(float d) noexcept { modDepth = clampf(d, 0.0f, 1.0f) * 0.0025f * static_cast<float>(sampleRate); } // up to 2.5 ms
    void setModRate(float mult) noexcept
    {
        for (int i = 0; i < kLines; ++i) lfo[i].setRate((0.09f + 0.031f * i) * clampf(mult, 0.1f, 8.0f));
    }
    void setFreeze(bool f) noexcept { freeze = f; }

    void process(float inL, float inR, float& outL, float& outR) noexcept
    {
        // Input: mono sum, highpassed, through 4 modulated allpasses → dense onset
        float x = loopHP.process(0.5f * (inL + inR));
        for (int i = 0; i < 4; ++i)
        {
            const float d = kInApMs[i] * 0.001f * static_cast<float>(sampleRate) + inLfo[i].next() * 0.0004f * static_cast<float>(sampleRate);
            x = allpassFrac(inAP[i], x, d, diffusion);
        }

        // Read the lines with slow modulation, then damp
        std::array<float, kLines> v;
        float sum = 0.0f;
        for (int i = 0; i < kLines; ++i)
        {
            const float d = baseDelay[i] + lfo[i].next() * modDepth;
            float s = lines[i].read(d);
            s = damp[i].process(s);
            s = lineHP[i].process(s);
            v[i] = s;
            sum += s;
        }

        // Householder feedback: y_i = v_i - (2/N)·Σv
        const float k = (2.0f / kLines) * sum;
        for (int i = 0; i < kLines; ++i)
        {
            const float g = freeze ? 0.9995f : gain[i];
            float y = (v[i] - k) * g + (freeze ? 0.0f : x * kInputSign[i] * 0.5f);
            if (freeze) y = fastTanh(y);          // only clamp when pinned near unity
            lines[i].write(y);
        }

        // Decorrelated taps + output allpasses
        float l = (v[0] - v[1] + v[2] - v[3] + v[4] - v[5] + v[6] - v[7]) * 0.5f;
        float r = (v[0] + v[1] - v[2] - v[3] + v[4] + v[5] - v[6] - v[7]) * 0.5f;
        l = allpassInt(outAP[0][0], l, 311, 0.5f); l = allpassInt(outAP[0][1], l, 467, 0.45f);
        r = allpassInt(outAP[1][0], r, 353, 0.5f); r = allpassInt(outAP[1][1], r, 419, 0.45f);
        outL = l; outR = r;
    }

private:
    void updateGains() noexcept
    {
        // g = 10^(-3 * delay_seconds / RT60)
        for (int i = 0; i < kLines; ++i)
        {
            const float dSec = baseDelay[i] / static_cast<float>(sampleRate);
            gain[i] = std::pow(10.0f, -3.0f * dSec / rt60);
        }
    }

    static float allpassFrac(DelayLine& dl, float x, float delaySamples, float g) noexcept
    {
        const float z = dl.read(delaySamples);
        const float w = x + z * g;
        dl.write(w);
        return z - w * g;
    }
    static float allpassInt(DelayLine& dl, float x, int d, float g) noexcept
    {
        const float z = dl.readInt(d);
        const float w = x + z * g;
        dl.write(w);
        return z - w * g;
    }

    // Base line lengths (ms) — mutually prime-ish, spread 1:3 for a smooth modal distribution
    static constexpr float kBaseMs[kLines]     = { 41.3f, 53.9f, 67.7f, 79.1f, 97.3f, 109.7f, 127.1f, 139.9f };
    static constexpr float kInApMs[4]          = { 7.9f, 13.1f, 21.7f, 31.3f };
    static constexpr float kInputSign[kLines]  = { 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f };

    double sampleRate = 44100.0;
    float size = 0.6f, rt60 = 3.0f, modDepth = 0.0f, diffusion = 0.65f;
    bool  freeze = false;
    std::array<DelayLine, kLines> lines;
    std::array<DelayLine, 4>      inAP;
    DelayLine                     outAP[2][2];
    std::array<OnePole, kLines>   damp, lineHP;
    std::array<LFO, kLines>       lfo;
    std::array<LFO, 4>            inLfo;
    std::array<float, kLines>     baseDelay {}, gain {};
    OnePole loopHP;
};

} // namespace zw
