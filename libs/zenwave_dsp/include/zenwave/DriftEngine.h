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
    float wobbleHz   = 0.45f;   // wow rate (free or tempo-derived)    // Wet-path EQ
    float lowCutHz   = 20.0f;   // 20..2000
    float highCutHz  = 20000.0f;// 1000..20000
    int   lowSlope   = 1;       // 0=6, 1=12, 2=24, 3=48 dB/oct
    int   highSlope  = 1;
    float tilt       = 0.0f;    // -1..1 (dark..bright)
    float color      = 0.0f;    // -1..1 feedback colour: negative = resonant lowpass sweeps down, positive = highpass thins out
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
            colorF[c].setSampleRate(sr);
            preHP[c].setSampleRate(sr);
            preHP[c].setHighpass(80.0f);
        }
        wf[1].setSampleRate(sr);   // re-seed phases differently for R
        reverb.prepare(sr);
        for (int c = 0; c < 2; ++c)
        {
            lowCut[c].setSampleRate(sr);  lowCut[c].setType(CutFilter::Type::Lowcut);
            highCut[c].setSampleRate(sr); highCut[c].setType(CutFilter::Type::Highcut);
            tiltEq[c].setSampleRate(sr);
        }
        for (auto* s : { &smTime, &smFeedback, &smMix, &smSpace, &smOut, &smWidth, &smFreeze })
        { s->setSampleRate(sr); s->setTime(30.0f); }
        smFreeze.reset(0.0f);
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

    // When the host runs the whole engine at 2x, the tape stage needs only 2x of its own (4x total)
    void setInternalOversampling(int factor) noexcept { for (auto& s : sat) s.setOversampling(factor); }

    void reset() noexcept
    {
        for (int c = 0; c < 2; ++c) { delay[c].reset(); ageLP[c].reset(); ageHP[c].reset(); sat[c].reset(); preHP[c].reset(); colorF[c].reset(); }
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
            wf[c].setWowRate(p.wobbleHz);
            ageLP[c].setLowpass(lpHz);
            sat[c].setDrive(p.age * 0.55f);
            sat[c].setEmphasis(p.age * 0.6f);
            sat[c].setHeadBump(p.age);
            colorF[c].setMode(p.color < 0.0f ? SVF::Mode::Lowpass : SVF::Mode::Highpass);
            colorF[c].setCutoff(p.color < 0.0f ? lerp(12000.0f, 600.0f, -p.color) : lerp(60.0f, 1500.0f, p.color));
            colorF[c].setResonance(0.7f + std::fabs(p.color) * 1.6f);
        }
        hissGain = p.age * p.age * 0.004f;
        reverb.setSize(p.size);
        reverb.setDecay(p.decay);
        reverb.setDamping(lerp(11000.0f, 3000.0f, p.age));
        reverb.setModDepth(0.35f + p.wowFlutter * 0.4f);
        reverb.setDiffusion(0.7f);
        reverb.setFreeze(p.freeze);        for (int c = 0; c < 2; ++c)
        {
            lowCut[c].setSlope(static_cast<CutFilter::Slope>(clampi(p.lowSlope, 0, 3)));
            highCut[c].setSlope(static_cast<CutFilter::Slope>(clampi(p.highSlope, 0, 3)));
            lowCut[c].setCutoff(p.lowCutHz);
            highCut[c].setCutoff(p.highCutHz);
            tiltEq[c].setTilt(p.tilt);
        }
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

            float rdL = delay[0].read(std::max(1.0f, tL));
            float rdR = delay[1].read(std::max(1.0f, tR));

            // Freeze seam blend: the captured loop has a natural discontinuity where "now" meets
            // "one loop ago". While frozen we soften that seam with a 10 ms raised-cosine dip.
            {
                const bool frozen = smFreeze.getTarget() > 0.5f;
                if (frozen && ! wasFrozen) { seamLoopLen = std::max(64.0f, t); seamPos = 0.0f; }
                wasFrozen = frozen;
                if (frozen)
                {
                    seamPos += 1.0f; if (seamPos >= seamLoopLen) seamPos -= seamLoopLen;
                    const float half = 0.005f * float(sampleRate);
                    const float d = std::min(seamPos, seamLoopLen - seamPos);   // distance to seam
                    if (d < half)
                    {
                        const float w = 0.5f * (1.0f + std::cos(kPi * d / half));   // 1 at seam → 0 at edge
                        const float gdip = 1.0f - 0.85f * w;
                        rdL *= gdip; rdR *= gdip;
                    }
                }
            }

            // Freeze glides: input fades out, loop gain glides to unity, aging fades out
            const float fz = smFreeze.next();

            // Age the repeats: tone, hiss, tape saturation (bypassed while frozen so the
            // captured audio recirculates transparently instead of squaring up)
            float agedL = lerp(ageAndSaturate(0, rdL), rdL, fz);
            float agedR = lerp(ageAndSaturate(1, rdR), rdR, fz);

            float inL = preHP[0].process(dryL) * (1.0f - fz);
            float inR = preHP[1].process(dryR) * (1.0f - fz);
            const float fbGain = lerp(fb, 1.0f, fz);

            float wL, wR;
            if (params.pingPong) { wL = inL + agedR * fbGain; wR = inR + agedL * fbGain; }
            else                 { wL = inL + agedL * fbGain; wR = inR + agedR * fbGain; }

            // Soft limit so feedback > 1 blooms instead of exploding (transparent when frozen)
            delay[0].write(lerp(fastTanh(wL), softCeiling(wL), fz));
            delay[1].write(lerp(fastTanh(wR), softCeiling(wR), fz));

            // Reverb send from the echo
            float rvL = 0.0f, rvR = 0.0f;
            // Send both the (highpassed) input and the repeats, so short notes still get space
            reverb.process((inL * 0.5f + agedL) * spc, (inR * 0.5f + agedR) * spc, rvL, rvR);

            float wetL = (agedL + rvL * 1.3f) * 0.8f;
            float wetR = (agedR + rvR * 1.3f) * 0.8f;

            // Wet EQ: cuts + tilt
            wetL = tiltEq[0].process(highCut[0].process(lowCut[0].process(wetL)));
            wetR = tiltEq[1].process(highCut[1].process(lowCut[1].process(wetR)));

            // Width (mid/side)
            const float mid  = 0.5f * (wetL + wetR);
            const float side = 0.5f * (wetL - wetR) * wid;
            wetL = mid + side; wetR = mid - side;

            // Soft ceiling keeps freeze / self-oscillation / long tails from spiking the master
            wetL = softCeiling(wetL); wetR = softCeiling(wetR);

            // Equal-power dry/wet
            const float a = std::cos(mix * kPi * 0.5f);
            const float b = std::sin(mix * kPi * 0.5f);
            L[i] = (dryL * a + wetL * b) * out;
            R[i] = (dryR * a + wetR * b) * out;
            if (wetTapL) { wetTapL[i] = wetL * b * out; wetTapR[i] = wetR * b * out; }
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
        if (std::fabs(params.color) > 0.001f) x = colorF[c].process(x);
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
    SVF        colorF[2];
    Saturator  sat[2];
    SpaceReverb reverb;
    Noise noise { 0x1234567u };
    float hissGain = 0.0f;
    float env[2] { 0.0f, 0.0f };
    bool  wasFrozen = false; float seamPos = 0.0f, seamLoopLen = 24000.0f;
    CutFilter lowCut[2], highCut[2];
    TiltEQ tiltEq[2];
    Smoother smTime, smFeedback, smMix, smSpace, smOut, smWidth, smFreeze;
public:
    // Optional: receives the post-EQ, post-mix wet signal (for analysers). Pointers valid for one process() call.
    void setWetTap(float* l, float* r) noexcept { wetTapL = l; wetTapR = r; }
private:
    float* wetTapL = nullptr; float* wetTapR = nullptr;
    float scratchR[8192] {};
};

} // namespace zw
