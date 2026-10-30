#pragma once
// ZenWave DSP — tape-style saturation stage.
// Asymmetric soft clip with bias (even harmonics), pre-emphasis and a DC blocker.
// Run it inside a JUCE dsp::Oversampling block for alias-free drive at mastering level.

#include "Utils.h"
#include "Filters.h"

namespace zw {

class Saturator
{
public:
    enum class Character { Tape, Tube, Hard };

    void setSampleRate(double sr) noexcept
    {
        dc.setSampleRate(sr);
        preEmphasis.setSampleRate(sr);  preEmphasis.setHighpass(1800.0f);
        deEmphasis.setSampleRate(sr);   deEmphasis.setLowpass(1800.0f);
        headBump.setSampleRate(sr);     headBump.setMode(SVF::Mode::Bandpass); headBump.setCutoff(95.0f); headBump.setResonance(0.9f);
        reset();
    }

    void reset() noexcept { dc.reset(); preEmphasis.reset(); deEmphasis.reset(); headBump.reset(); os2.reset(); os4.reset(); }
    void setHeadBump(float amt) noexcept { bump = clampf(amt, 0.0f, 1.0f); }
    // factor: 1, 2 or 4
    void setOversampling(int factor) noexcept { osFactor = factor >= 4 ? 4 : (factor >= 2 ? 2 : 1); }

    // drive: 0..1 (maps to 0..+24 dB), bias: -1..1, character selects the curve
    void setDrive(float d) noexcept
    {
        drive = clampf(d, 0.0f, 1.0f);
        gain  = dbToGain(drive * 24.0f);
        // Gain law: near-unity for small signals (so a feedback loop's gain stays honest),
        // gentle level compensation for loud ones. out ≈ tanh(g·x) / g^0.85
        makeup = 1.0f / std::pow(std::max(1.0f, gain), 0.85f);
    }
    void setBias(float b) noexcept { bias = clampf(b, -1.0f, 1.0f) * 0.25f; }
    void setCharacter(Character c) noexcept { character = c; }
    void setEmphasis(float amt) noexcept { emphasis = clampf(amt, 0.0f, 1.0f); }

    float process(float x) noexcept
    {
        // Tape: pre-emphasis (HF lift into the curve) → nonlinearity → matching de-emphasis.
        // Louder = more HF compression, exactly how tape behaves. Head bump adds low-mid weight.
        const float hf  = preEmphasis.process(x);
        const float pre = (x + hf * emphasis) * gain + bias;

        float v;
        if (osFactor == 4)
        {
            float y[4]; os4.upsample(pre, y);
            for (float& q : y) q = curve(q);
            v = os4.downsample(y);
        }
        else if (osFactor == 2)
        {
            float a, b; os2.upsample(pre, a, b);
            v = os2.downsample(curve(a), curve(b));
        }
        else v = curve(pre);

        // De-emphasis: blend toward the lowpassed version by the same amount we lifted
        const float lp = deEmphasis.process(v);
        v = lerp(v, lp, emphasis / (1.0f + emphasis));
        v += headBump.process(v) * bump * 0.35f;
        v  = dc.process(v);
        return v * makeup;
    }

private:
    float curve(float v) const noexcept
    {
        switch (character)
        {
            case Character::Tape: return fastTanh(v);
            case Character::Tube: return tube(v);
            case Character::Hard: return hard(v);
        }
        return v;
    }
    static float tube(float v) noexcept
    {
        // Asymmetric: softer on positive swing, harder on negative
        return v >= 0.0f ? fastTanh(v * 0.8f) * 1.1f : fastTanh(v * 1.3f) * 0.85f;
    }
    static float hard(float v) noexcept
    {
        // Cubic soft-knee into hard clip
        const float t = clampf(v, -1.5f, 1.5f);
        return t - (t * t * t) / 6.75f;
    }

    float drive = 0.0f, gain = 1.0f, makeup = 1.0f, bias = 0.0f, emphasis = 0.3f;
    Character character = Character::Tape;
    int osFactor = 4;
    float bump = 0.0f;
    DCBlocker dc;
    OnePole   preEmphasis, deEmphasis;
    SVF       headBump;
    Oversampler2x os2;
    Oversampler4x os4;
};

} // namespace zw
