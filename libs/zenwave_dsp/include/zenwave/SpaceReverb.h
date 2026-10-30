#pragma once
// ZenWave DSP — SpaceReverb: an 8-line feedback delay network with Householder
// mixing, input diffusion and in-loop damping. Designed for long, smooth, "off-world"
// tails that stay clean under sub-heavy material (the loop is highpassed).

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
            lines[i].prepare(sr, 0.5f);
            damp[i].setSampleRate(sr);
            damp[i].setLowpass(6000.0f);
            lfo[i].setSampleRate(sr);
            lfo[i].setRate(0.07f + 0.013f * static_cast<float>(i));
            lfo[i].setShape(LFO::Shape::Sine);
            lfo[i].reset(static_cast<float>(i) / kLines);
        }
        for (int i = 0; i < 4; ++i) diffusers[i].prepare(sr, 0.05f);
        loopHP.setSampleRate(sr); loopHP.setHighpass(90.0f);
        setSize(0.6f);
        setDecay(0.6f);
        reset();
    }

    void reset() noexcept
    {
        for (auto& l : lines) l.reset();
        for (auto& d : diffusers) d.reset();
        for (auto& f : damp) f.reset();
        loopHP.reset();
        state.fill(0.0f);
    }

    // size 0..1 scales the delay lengths (room → cathedral → void)
    void setSize(float s) noexcept
    {
        size = clampf(s, 0.0f, 1.0f);
        const float scale = 0.25f + size * 1.75f;
        for (int i = 0; i < kLines; ++i)
            baseDelay[i] = kPrimesMs[i] * 0.001f * static_cast<float>(sampleRate) * scale;
    }

    // decay 0..1 → feedback gain; 1.0 is near-infinite (use with Freeze)
    void setDecay(float d) noexcept { feedback = 0.3f + clampf(d, 0.0f, 1.0f) * 0.69f; }
    void setDamping(float hz) noexcept { for (auto& f : damp) f.setLowpass(hz); }
    void setLoopHighpass(float hz) noexcept { loopHP.setHighpass(hz); }
    void setDiffusion(float g) noexcept { diffusion = clampf(g, 0.0f, 0.85f); }
    void setModDepth(float d) noexcept { modDepth = clampf(d, 0.0f, 1.0f) * 6.0f; }  // samples
    void setModRate(float mult) noexcept
    {
        for (int i = 0; i < kLines; ++i) lfo[i].setRate((0.07f + 0.013f * static_cast<float>(i)) * clampf(mult, 0.1f, 8.0f));
    }
    void setFreeze(bool f) noexcept { freeze = f; }

    void process(float inL, float inR, float& outL, float& outR) noexcept
    {
        // Mono-ish input with a little width, through 4 series allpasses
        float x = 0.5f * (inL + inR);
        x = loopHP.process(x);
        for (int i = 0; i < 4; ++i) x = allpass(diffusers[i], x, kDiffMs[i], diffusion);

        // Read all lines (modulated), damp, collect
        std::array<float, kLines> v;
        for (int i = 0; i < kLines; ++i)
        {
            const float d = baseDelay[i] + lfo[i].next() * modDepth;
            v[i] = damp[i].process(lines[i].read(d));
        }

        // Householder mixing: y = v - (2/N) * sum(v)
        float sum = 0.0f;
        for (float s : v) sum += s;
        const float k = 2.0f / kLines * sum;

        const float fb = freeze ? 0.999f : feedback;
        const float inject = freeze ? 0.0f : x;

        for (int i = 0; i < kLines; ++i)
        {
            float y = (v[i] - k) * fb + inject * kInputGain[i];
            y = fastTanh(y * 0.9f) * 1.1111f;      // gentle limiter keeps freeze stable
            lines[i].write(y);
        }

        outL = (v[0] + v[2] + v[4] + v[6]) * 0.35f;
        outR = (v[1] + v[3] + v[5] + v[7]) * 0.35f;
    }

private:
    // Schroeder allpass on a delay line
    static float allpass(DelayLine& dl, float x, float ms, float g) noexcept
    {
        const int   d = static_cast<int>(ms * 44.1f);   // fixed-ish; fine for diffusion
        const float z = dl.readInt(d);
        const float w = x + z * g;
        dl.write(w);
        return z - w * g;
    }

    static constexpr float kPrimesMs[kLines]  = { 29.7f, 37.1f, 41.3f, 47.9f, 53.3f, 61.7f, 71.1f, 79.3f };
    static constexpr float kDiffMs[4]         = { 4.7f, 7.3f, 11.9f, 17.1f };
    static constexpr float kInputGain[kLines] = { 1.0f, -1.0f, 1.0f, -1.0f, 0.7f, -0.7f, 0.7f, -0.7f };

    double sampleRate = 44100.0;
    float size = 0.6f, feedback = 0.7f, modDepth = 2.0f, diffusion = 0.6f;
    bool  freeze = false;
    std::array<DelayLine, kLines> lines;
    std::array<DelayLine, 4>      diffusers;
    std::array<OnePole, kLines>   damp;
    std::array<LFO, kLines>       lfo;
    std::array<float, kLines>     baseDelay {};
    std::array<float, kLines>     state {};
    OnePole loopHP;
};

} // namespace zw
