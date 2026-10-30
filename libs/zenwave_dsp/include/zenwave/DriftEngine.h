#pragma once
// ZenWave DSP — DriftEngine: the lo-fi space echo behind the free "Drift" plugin.
//
// Signal flow (per channel, stereo):
//   in → [pre-HP] → + → tape delay (wow/flutter) → age (LP + hiss + saturation) → feedback ┐
//                   └──────────────────────────────────────────────────────────────────────┘
//   echo → SpaceReverb (send) → mix with dry, width control, output trim
//
// JUCE-free so it can be unit-tested and reused (Umbra and Halo share the reverb).

#include "Utils.h"
#include "Filters.h"
#include "DelayLine.h"
#include "Modulation.h"
#include "Saturator.h"
#include "SpaceReverb.h"

namespace zw {

struct DriftParams
{
    float timeMs     = 380.0f;  // 20..2000
    float feedback   = 0.45f;   // 0..1.1 (above 1 self-oscillates, limited)
    float wowFlutter = 0.3f;    // 0..1
    float age        = 0.4f;    // 0..1  (darker, hissier, more saturated)
    float space      = 0.35f;   // 0..1  reverb send
    float size       = 0.6f;    // 0..1  reverb size
    float decay      = 0.6f;    // 0..1  reverb decay
    float width      = 1.0f;    // 0..1.5
    float mix        = 0.35f;   // 0..1
    float output     = 0.0f;    // dB, -24..+12
    bool  pingPong   = true;
    bool  freeze     = false;
    float spread     = 0.0f;    // -1..1 L/R time offset
};

class DriftEngine
{
public:
    void prepare(double sr, int /*maxBlock*/) noexcept
    {
        sampleRate = sr;
        for (int c = 0; c < 2; ++c)
        {
            delay[c].prepare(sr, 2.5f);
            wf[c].setSampleRate(sr);
            ageLP[c].setSampleRate(sr);
            ageHP[c].setSampleRate(sr);
            ageHP[c].setHighpass(60.0f);
            sat[c].setSampleRate(sr);
            sat[c].setCharacter(Saturator::Character::Tape);
            preHP[c].setSampleRate(sr);
            preHP[c].setHighpass(80.0f);
        }
        wf[1].setSampleRate(sr);   // re-seed phases differently for R
        reverb.prepare(sr);
        for (auto* s : { &smTime, &smFeedback, &smMix, &smSpace, &smOut, &smWidth })
        { s->setSampleRate(sr); s->setTime(30.0f); }
        smTime.setTime(120.0f);     // slower: avoids pitch zips on big time jumps
        setParams(params);
        smTime.reset(msToSamples(params.timeMs));
        smFeedback.reset(params.feedback);
        smMix.reset(params.mix);
        smSpace.reset(params.space);
        smOut.reset(dbToGain(params.output));
        smWidth.reset(params.width);
        reset();
    }

    void reset() noexcept
    {
        for (int c = 0; c < 2; ++c) { delay[c].reset(); ageLP[c].reset(); ageHP[c].reset(); sat[c].reset(); preHP[c].reset(); }
        reverb.reset();
    }

    void setParams(const DriftParams& p) noexcept
    {
        params = p;
        smTime.setTarget(msToSamples(p.timeMs));
        smFeedback.setTarget(clampf(p.feedback, 0.0f, 1.1f));
        smMix.setTarget(p.mix);
        smSpace.setTarget(p.space);
        smOut.setTarget(dbToGain(p.output));
        smWidth.setTarget(p.width);

        const float lpHz = lerp(16000.0f, 1800.0f, p.age * p.age);
        for (int c = 0; c < 2; ++c)
        {
            wf[c].setAmount(p.wowFlutter);
            ageLP[c].setLowpass(lpHz);
            sat[c].setDrive(p.age * 0.7f);
            sat[c].setEmphasis(p.age);
        }
        hissGain = p.age * p.age * 0.004f;
        reverb.setSize(p.size);
        reverb.setDecay(p.decay);
        reverb.setDamping(lerp(9000.0f, 2500.0f, p.age));
        reverb.setFreeze(p.freeze);
    }

    void process(float* L, float* R, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            const float t   = smTime.next();
            const float fb  = smFeedback.next();
            const float mix = smMix.next();
            const float spc = smSpace.next();
            const float out = smOut.next();
            const float wid = smWidth.next();

            // Per-channel delay times with optional spread
            const float tL = t * (1.0f - params.spread * 0.25f) + wf[0].nextOffsetSamples();
            const float tR = t * (1.0f + params.spread * 0.25f) + wf[1].nextOffsetSamples();

            const float rdL = delay[0].read(std::max(1.0f, tL));
            const float rdR = delay[1].read(std::max(1.0f, tR));

            // Age the repeats: tone, hiss, tape saturation
            float agedL = ageAndSaturate(0, rdL);
            float agedR = ageAndSaturate(1, rdR);

            // Feedback routing
            float inL = params.freeze ? 0.0f : preHP[0].process(dryL);
            float inR = params.freeze ? 0.0f : preHP[1].process(dryR);
            const float fbGain = params.freeze ? 1.0f : fb;

            float wL, wR;
            if (params.pingPong) { wL = inL + agedR * fbGain; wR = inR + agedL * fbGain; }
            else                 { wL = inL + agedL * fbGain; wR = inR + agedR * fbGain; }

            // Soft limit so feedback > 1 blooms instead of exploding
            delay[0].write(fastTanh(wL));
            delay[1].write(fastTanh(wR));

            // Reverb send from the echo
            float rvL = 0.0f, rvR = 0.0f;
            reverb.process(agedL * spc, agedR * spc, rvL, rvR);

            float wetL = agedL + rvL;
            float wetR = agedR + rvR;

            // Width (mid/side)
            const float mid  = 0.5f * (wetL + wetR);
            const float side = 0.5f * (wetL - wetR) * wid;
            wetL = mid + side; wetR = mid - side;

            // Equal-power dry/wet
            const float a = std::cos(mix * kPi * 0.5f);
            const float b = std::sin(mix * kPi * 0.5f);
            L[i] = (dryL * a + wetL * b) * out;
            R[i] = (dryR * a + wetR * b) * out;
        }
    }

    // Convenience for mono hosts
    void processMono(float* M, int n) noexcept
    {
        for (int i = 0; i < n; ++i) scratchR[i] = M[i];
        process(M, scratchR, n);
        for (int i = 0; i < n; ++i) M[i] = 0.5f * (M[i] + scratchR[i]);
    }

private:
    float ageAndSaturate(int c, float x) noexcept
    {
        x = ageLP[c].process(x);
        x = ageHP[c].process(x);
        // Tape hiss rides on the signal (ducked when the tape is "quiet"),
        // so silence stays silent and busy repeats get grit.
        env[c] += (std::fabs(x) - env[c]) * (std::fabs(x) > env[c] ? 0.01f : 0.0005f);
        const float gate = std::min(1.0f, env[c] * 40.0f);
        x += noise.next() * hissGain * gate;
        return sat[c].process(x);
    }

    float msToSamples(float ms) const noexcept
    {
        return clampf(ms, 1.0f, 2400.0f) * 0.001f * static_cast<float>(sampleRate);
    }

    double sampleRate = 44100.0;
    DriftParams params;
    DelayLine  delay[2];
    WowFlutter wf[2];
    OnePole    ageLP[2], ageHP[2], preHP[2];
    Saturator  sat[2];
    SpaceReverb reverb;
    Noise noise { 0x1234567u };
    float hissGain = 0.0f;
    float env[2] { 0.0f, 0.0f };
    Smoother smTime, smFeedback, smMix, smSpace, smOut, smWidth;
    float scratchR[8192] {};
};

} // namespace zw
