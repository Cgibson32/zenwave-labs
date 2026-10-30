#pragma once
// ZenWave DSP — UmbraEngine: the dark reverb behind "Umbra" ($49).
//
//   in → GRAVITY crossover (sub stays dry) → pre-delay → early reflections ─┐
//        ─────────────────────────────────→ SpaceReverb (late) ────────────┴→ DARK tilt → DUCK → WIDTH → MIX
//
// "Dark" is the identity: a tilt filter in the wet path plus in-loop damping, so the
// tail sinks behind the mix instead of fighting it. Gravity sets a crossover below
// which the input never enters the reverb, so basslines and subs stay tight.

#include "Utils.h"
#include "Filters.h"
#include "DelayLine.h"
#include "SpaceReverb.h"

namespace zw {

struct UmbraParams
{
    float preDelayMs = 20.0f;   // 0..250
    float size       = 0.65f;   // 0..1
    float decay      = 0.6f;    // 0..1
    float dark       = 0.5f;    // 0..1   tone: 0 = open, 1 = pitch black
    float density    = 0.6f;    // 0..1   diffusion
    float motion     = 0.3f;    // 0..1   modulation depth
    float gravity    = 0.4f;    // 0..1   crossover 40..300 Hz below which input bypasses the verb
    float early      = 0.3f;    // 0..1   early reflection level
    float duck       = 0.0f;    // 0..1   sidechain ducking of the wet signal by the dry input
    float width      = 1.0f;    // 0..1.5
    float mix        = 0.3f;    // 0..1
    float output     = 0.0f;    // dB
    bool  freeze     = false;
    float motionRate = 1.0f;    // multiplier on the reverb LFO rates (free or tempo-derived)
};

class UmbraEngine
{
public:
    void prepare(double sr, int /*maxBlock*/) noexcept
    {
        sampleRate = sr;
        reverb.prepare(sr);
        for (int c = 0; c < 2; ++c)
        {
            pre[c].prepare(sr, 0.3f);
            er[c].prepare(sr, 0.12f);
            xoverLP[c].setSampleRate(sr); xoverLP[c].setMode(SVF::Mode::Lowpass);  xoverLP[c].setResonance(0.5f);
            xoverHP[c].setSampleRate(sr); xoverHP[c].setMode(SVF::Mode::Highpass); xoverHP[c].setResonance(0.5f);
            tiltLP[c].setSampleRate(sr);
            tiltHP[c].setSampleRate(sr);
            tiltHP[c].setHighpass(800.0f);
        }
        for (auto* s : { &smMix, &smOut, &smWidth, &smPre, &smEarly }) { s->setSampleRate(sr); s->setTime(30.0f); }
        smPre.setTime(80.0f);
        setParams(params);
        smMix.reset(params.mix); smOut.reset(dbToGain(params.output)); smWidth.reset(params.width);
        smPre.reset(params.preDelayMs * 0.001f * float(sr)); smEarly.reset(params.early);
        duckEnv = 0.0f;
        reset();
    }

    void reset() noexcept
    {
        reverb.reset();
        for (int c = 0; c < 2; ++c) { pre[c].reset(); er[c].reset(); xoverLP[c].reset(); xoverHP[c].reset(); tiltLP[c].reset(); tiltHP[c].reset(); }
    }

    void setParams(const UmbraParams& p) noexcept
    {
        params = p;
        reverb.setSize(p.size);
        reverb.setDecay(p.decay);
        reverb.setDiffusion(p.density);
        reverb.setModDepth(p.motion);
        reverb.setModRate(p.motionRate);
        reverb.setFreeze(p.freeze);
        // Dark: in-loop damping 12 kHz → 1.2 kHz, plus wet-path tilt
        reverb.setDamping(lerp(12000.0f, 1200.0f, p.dark * p.dark));
        tiltAmount = p.dark;
        const float xo = lerp(40.0f, 300.0f, p.gravity);
        for (int c = 0; c < 2; ++c)
        {
            xoverLP[c].setCutoff(xo); xoverHP[c].setCutoff(xo);
            tiltLP[c].setLowpass(lerp(14000.0f, 2500.0f, p.dark));
        }
        reverb.setLoopHighpass(std::max(60.0f, xo * 0.8f));
        smMix.setTarget(p.mix);
        smOut.setTarget(dbToGain(p.output));
        smWidth.setTarget(p.width);
        smPre.setTarget(p.preDelayMs * 0.001f * float(sampleRate));
        smEarly.setTarget(p.early);
        duckAmount = p.duck;
        duckAttack  = 1.0f - std::exp(-1.0f / (0.005f * float(sampleRate)));
        duckRelease = 1.0f - std::exp(-1.0f / (0.25f  * float(sampleRate)));
    }

    void process(float* L, float* R, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            const float mix = smMix.next(), out = smOut.next(), wid = smWidth.next();
            const float preS = smPre.next(), erLvl = smEarly.next();

            // Gravity crossover: lows bypass, highs feed the verb
            const float lowL = xoverLP[0].process(dryL), lowR = xoverLP[1].process(dryR);
            const float hiL  = xoverHP[0].process(dryL), hiR  = xoverHP[1].process(dryR);

            // Pre-delay
            pre[0].write(hiL); pre[1].write(hiR);
            const float pL = pre[0].read(std::max(1.0f, preS));
            const float pR = pre[1].read(std::max(1.0f, preS));

            // Early reflections: 6 taps, scaled by size
            er[0].write(pL); er[1].write(pR);
            const float sz = 0.4f + params.size * 0.6f;
            float eL = 0.0f, eR = 0.0f;
            for (int t = 0; t < 6; ++t)
            {
                const float d = kErMs[t] * sz * 0.001f * float(sampleRate);
                eL += er[0].read(d) * kErGain[t];
                eR += er[1].read(d) * kErGain[(t + 3) % 6];
            }

            // Late reverb
            float lL = 0.0f, lR = 0.0f;
            reverb.process(pL + eL * 0.3f, pR + eR * 0.3f, lL, lR);

            float wetL = lL * 1.4f + eL * erLvl * 0.6f;
            float wetR = lR * 1.4f + eR * erLvl * 0.6f;

            // Dark tilt: cut highs, nudge low-mids
            wetL = darken(0, wetL); wetR = darken(1, wetR);

            // Duck wet by dry level
            const float level = std::max(std::fabs(dryL), std::fabs(dryR));
            duckEnv += (level - duckEnv) * (level > duckEnv ? duckAttack : duckRelease);
            const float duckGain = 1.0f - duckAmount * std::min(1.0f, duckEnv * 2.5f);
            wetL *= duckGain; wetR *= duckGain;

            // Width
            const float mid = 0.5f * (wetL + wetR), side = 0.5f * (wetL - wetR) * wid;
            wetL = mid + side; wetR = mid - side;

            // Equal-power mix; sub band always passes dry
            const float a = std::cos(mix * kPi * 0.5f), b = std::sin(mix * kPi * 0.5f);
            L[i] = ((hiL * a + wetL * b) + lowL) * out;
            R[i] = ((hiR * a + wetR * b) + lowR) * out;
        }
    }

    void processMono(float* M, int n) noexcept
    {
        for (int i = 0; i < n; ++i) scratch[i] = M[i];
        process(M, scratch, n);
        for (int i = 0; i < n; ++i) M[i] = 0.5f * (M[i] + scratch[i]);
    }

private:
    float darken(int c, float x) noexcept
    {
        const float lp = tiltLP[c].process(x);
        const float hp = tiltHP[c].process(x);
        // Blend toward the lowpassed signal, pull a little high content down
        return lerp(x, lp, tiltAmount * 0.8f) - hp * tiltAmount * 0.25f;
    }

    static constexpr float kErMs[6]   = { 11.0f, 19.0f, 27.0f, 41.0f, 53.0f, 67.0f };
    static constexpr float kErGain[6] = { 0.80f, -0.65f, 0.55f, -0.45f, 0.35f, -0.25f };

    double sampleRate = 44100.0;
    UmbraParams params;
    SpaceReverb reverb;
    DelayLine pre[2], er[2];
    SVF xoverLP[2], xoverHP[2];
    OnePole tiltLP[2], tiltHP[2];
    float tiltAmount = 0.5f;
    float duckAmount = 0.0f, duckEnv = 0.0f, duckAttack = 0.01f, duckRelease = 0.0001f;
    Smoother smMix, smOut, smWidth, smPre, smEarly;
    float scratch[8192] {};
};

} // namespace zw
