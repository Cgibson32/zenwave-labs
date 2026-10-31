#pragma once
// CORE: three-band saturator / distortion for bass music.
//
//   in → LR4 crossover (LOW | MID | HIGH) → per band: M/S focus → shaper (4x OS) → auto-gain
//      → band mix/gain → sum → width → wet EQ → soft ceiling → mix with dry → output
//
// SUB LOCK keeps the low band clean and mono. Auto-gain matches each band's wet level to its
// dry level so turning DRIVE up never fools you with loudness.

#include "Utils.h"
#include "Filters.h"
#include <cmath>
#include <array>

namespace zw {

struct CoreBand
{
    int   character = 0;    // 0 Tape, 1 Tube, 2 Fold, 3 Rectify, 4 Crush, 5 Diode, 6 Octave, 7 Hard
    float drive = 0.3f;     // 0..1
    float mix   = 1.0f;     // 0..1
    float gain  = 0.0f;     // dB
    bool  enabled = true;   // off = band passes clean
    bool  solo = false;     // any solo = other bands muted
    float tuneHz = 1000.0f; // resonance centre (clamped inside the band)
    float res = 0.0f;       // 0..1 resonant pre-emphasis into the shaper (de-emphasised after)
};

struct CoreCrush
{
    bool  on = false;
    float bits = 12.0f;     // 2..16
    float rate = 0.0f;      // 0..1 sample-rate reduction (0 = none)
    float mix  = 1.0f;      // 0..1
};

struct CoreParams
{
    float drive = 0.4f;     // 0..1 global, multiplies band drives
    float heat  = 0.3f;     // 0..1 asymmetry / even harmonics + presence into the shaper
    float focus = 0.0f;     // -1 mid only .. 0 both .. +1 side only
    float width = 1.0f;     // 0..1.5
    float mix   = 1.0f;     // 0..1
    float output = 0.0f;    // dB
    float lowX  = 150.0f;   // 40..400
    float highX = 3500.0f;  // 1000..12000
    bool  subLock = true;
    bool  vowel = false;    // TUNE morphs vowel formants (A E I O U) instead of a single peak
    float punch = 0.0f;     // -1 = transients stay clean (saturate sustain) .. +1 = sustain clean (saturate transients)
    CoreBand band[3];
    CoreCrush crush;
    // wet-path EQ
    float lowCutHz = 20.0f, highCutHz = 20000.0f; int lowSlope = 1, highSlope = 1; float tilt = 0.0f;
};

// 4th-order Linkwitz-Riley crossover from two cascaded TPT 2nd-order Butterworth sections.
// Returns LP and HP that sum flat (allpass).
class CrossoverLR4
{
public:
    void setSampleRate(double sr) noexcept { fs = sr; update(); }
    void setCutoff(float hz) noexcept { fc = clampf(hz, 20.0f, float(fs * 0.45)); update(); }
    void reset() noexcept { for (auto& s : st) s = {}; }
    void process(float x, float& lp, float& hp) noexcept
    {
        float l1, h1; tick(st[0], x, l1, h1);
        float l2, h2, l3, h3;
        tick(st[1], l1, l2, h2);      // LP·LP
        tick(st[2], h1, l3, h3);      // HP·HP
        lp = l2; hp = h3;
    }
    // Allpass with the same phase as lp+hp (apply to other bands so everything stays aligned)
    float allpass(float x) noexcept { float l, h; process(x, l, h); return l + h; }
private:
    struct S { float ic1 = 0, ic2 = 0; };
    void tick(S& s, float v0, float& lp, float& hp) noexcept
    {
        const float v3 = v0 - s.ic2;
        const float v1 = a1 * s.ic1 + a2 * v3;
        const float v2 = s.ic2 + a2 * s.ic1 + a3 * v3;
        s.ic1 = 2 * v1 - s.ic1; s.ic2 = 2 * v2 - s.ic2;
        lp = v2; hp = v0 - k * v1 - v2;
    }
    void update() noexcept
    {
        g = std::tan(3.14159265f * fc / float(fs)); k = 1.41421356f;   // Butterworth Q = 0.7071
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    double fs = 48000.0; float fc = 1000.0f, g = 0, k = 1.414f, a1 = 0, a2 = 0, a3 = 0;
    S st[3];
};

// One band's shaper: 4x oversampled waveshaper with character, heat (asymmetry) and auto-gain.
class BandShaper
{
public:
    void prepare(double sr) noexcept
    {
        fs = sr; os.reset(); dc.setSampleRate(sr); dc.setHighpass(12.0f); pres.setSampleRate(sr); pres.setHighpass(1800.0f);
        envIn = envOut = 1e-4f; agGain = 1.0f; crushHold = 0.0f; crushPhase = 0.0f;
        const float a = 1.0f - std::exp(-1.0f / (0.05f * float(sr))); envA = a; agA = 1.0f - std::exp(-1.0f / (0.12f * float(sr)));
    }
    void reset() noexcept { os.reset(); dc.reset(); pres.reset(); bpPre.reset(); bpPost.reset(); bpPre2.reset(); bpPost2.reset(); }
    void set(int character, float drive, float heat) noexcept { ch = character; d = drive; h = heat; }
    // Targets only; the filters glide there (log-domain, ~12 ms) inside process() so modulated TUNE never clicks
    void setResonance(float hz, float amount, float hz2 = 0.0f) noexcept
    {
        tgtAmt = amount * amount * 6.0f;                    // up to +17 dB of peak into the shaper
        tgtQ = 0.8f + amount * 5.0f;
        tgtHz = hz; res2 = hz2 > 0.0f; tgtHz2 = res2 ? hz2 : hz;
        if (curHz <= 0.0f) { curHz = tgtHz; curHz2 = tgtHz2; curQ = tgtQ; resAmt = tgtAmt; applyRes(); }
    }

    float process(float x) noexcept
    {
        // track dry level
        envIn += (std::fabs(x) - envIn) * envA;
        if (d < 0.002f) { envOut = envIn; agGain = 1.0f; return x; }
        // glide resonance targets and refresh coefficients every 16 samples
        if ((++resCounter & 15) == 0)
        {
            const float k = 0.08f;   // 16-sample steps -> ~12 ms
            curHz = std::exp(std::log(curHz) + (std::log(tgtHz) - std::log(curHz)) * k);
            curHz2 = std::exp(std::log(curHz2) + (std::log(tgtHz2) - std::log(curHz2)) * k);
            curQ += (tgtQ - curQ) * k; resAmt += (tgtAmt - resAmt) * k;
            applyRes();
        }
        // resonant pre-emphasis: the chosen bandwidth hits the shaper harder (de-emphasised below)
        if (resAmt > 0.001f) { const float x0 = x; x += bpPre.process(x0) * resAmt; if (res2) x += bpPre2.process(x0) * resAmt * 0.7f; }

        const float pre = 1.0f + d * d * 30.0f;               // up to +30 dB into the shaper
        const float bias = h * 0.35f * d;                     // asymmetry = even harmonics
        float in = x + pres.process(x) * h * 0.6f;            // heat adds presence before the clip
        float y;
        if (ch == 4)                                          // Crush: deliberately aliasy, no OS
        {
            const float bits = lerp(16.0f, 4.0f, d), q = std::pow(2.0f, bits - 1.0f);
            const float rate = lerp(1.0f, 0.12f, d);         // sample-rate reduction
            crushPhase += rate; if (crushPhase >= 1.0f) { crushPhase -= 1.0f; crushHold = std::round(in * q) / q; }
            y = crushHold;
        }
        else
        {
            float up[4]; os.upsample(in, up);
            for (auto& v : up) v = shape((v + bias) * pre) - shape(bias * pre);
            y = os.downsample(up);
            y = dc.process(y);
        }
        if (resAmt > 0.001f) { const float y0 = y; y -= bpPost.process(y0) * (resAmt / (1.0f + resAmt)); if (res2) y -= bpPost2.process(y0) * (0.7f * resAmt / (1.0f + 0.7f * resAmt)); }   // de-emphasis
        // auto-gain: match wet level to dry level, slow, bounded
        envOut += (std::fabs(y) - envOut) * envA;
        const float want = clampf(envIn / std::max(envOut, 1e-5f), 0.08f, 2.5f);
        agGain += (want - agGain) * agA;
        return y * agGain;
    }
    float autoGainDb() const noexcept { return 20.0f * std::log10(std::max(agGain, 1e-4f)); }

private:
    void applyRes() noexcept
    {
        bpPre.setSampleRate(fs);  bpPre.setMode(SVF::Mode::Bandpass);  bpPre.setCutoff(curHz);  bpPre.setResonance(curQ);
        bpPost.setSampleRate(fs); bpPost.setMode(SVF::Mode::Bandpass); bpPost.setCutoff(curHz); bpPost.setResonance(curQ);
        if (res2)
        {
            bpPre2.setSampleRate(fs);  bpPre2.setMode(SVF::Mode::Bandpass);  bpPre2.setCutoff(curHz2); bpPre2.setResonance(curQ * 1.3f);
            bpPost2.setSampleRate(fs); bpPost2.setMode(SVF::Mode::Bandpass); bpPost2.setCutoff(curHz2); bpPost2.setResonance(curQ * 1.3f);
        }
    }
    float shape(float v) const noexcept
    {
        switch (ch)
        {
            case 0:  return fastTanh(v);                                                   // Tape
            case 1:  { const float t = fastTanh(v * 0.7f); return t + 0.18f * t * t * (v > 0 ? 1.0f : -1.0f); } // Tube: soft + 2nd
            case 2:  return std::sin(clampf(v, -6.2f, 6.2f) * 0.9f) * 0.95f;                // Fold
            case 3:  { const float r = v >= 0 ? v : -v * 0.35f; return fastTanh(r * 1.2f) - 0.25f; }   // Rectify (half-wave-ish)
            case 5:  { const float a = v >= 0 ? 1.0f : 2.2f; return fastTanh(v * a) / (v >= 0 ? 1.0f : 1.6f); }     // Diode: asymmetric knee, buzzy
            case 6:  { const float r = std::fabs(v); return fastTanh(r * 1.5f) * 0.9f - 0.45f; }                  // Octave: full-wave = octave-up fuzz
            case 7:  return clampf(v, -0.8f, 0.8f) * 1.1f;                                                         // Hard: digital clip
            default: return fastTanh(v);
        }
    }
    Oversampler4x os; OnePole dc, pres; SVF bpPre, bpPost, bpPre2, bpPost2; float resAmt = 0.0f; bool res2 = false;
    float tgtHz = 1000.0f, tgtHz2 = 1000.0f, tgtQ = 1.0f, tgtAmt = 0.0f, curHz = 0.0f, curHz2 = 1000.0f, curQ = 1.0f; unsigned resCounter = 0;
    int ch = 0; float d = 0, h = 0; double fs = 48000.0;
    float envIn = 0, envOut = 0, envA = 0.001f, agGain = 1.0f, agA = 0.001f, crushHold = 0, crushPhase = 0;
};

class CoreEngine
{
public:
    void prepare(double sr, int /*maxBlock*/) noexcept
    {
        fs = sr;
        for (int c = 0; c < 2; ++c)
        {
            xLow[c].setSampleRate(sr); xHigh[c].setSampleRate(sr); apLow[c].setSampleRate(sr);
            lowCut[c].setSampleRate(sr);  lowCut[c].setType(CutFilter::Type::Lowcut);
            highCut[c].setSampleRate(sr); highCut[c].setType(CutFilter::Type::Highcut);
            tiltEq[c].setSampleRate(sr);
        }
        for (int b = 0; b < 3; ++b) for (int ms = 0; ms < 2; ++ms) shaper[b][ms].prepare(sr);
        for (auto* s : { &smMix, &smOut, &smWidth, &smSub, &smFocus, &smHeat, &smLowX, &smHighX, &smCrushMix, &smPunch }) { s->setSampleRate(sr); s->setTime(30.0f); }
        tFast = 1.0f - std::exp(-1.0f / (0.0015f * float(sr))); tSlow = 1.0f - std::exp(-1.0f / (0.06f * float(sr))); envFast = envSlow = 0.0f;
        for (auto& s : smDrive) { s.setSampleRate(sr); s.setTime(30.0f); }
        for (auto& s : smBandMix) { s.setSampleRate(sr); s.setTime(30.0f); }
        for (auto& s : smBandGain) { s.setSampleRate(sr); s.setTime(30.0f); }
        setParams(params);
        smMix.reset(params.mix); smOut.reset(dbToGain(params.output)); smWidth.reset(params.width); smSub.reset(params.subLock ? 1.0f : 0.0f); smFocus.reset(params.focus);
        for (int b = 0; b < 3; ++b) { smBandMix[b].reset(params.band[b].mix); smBandGain[b].reset(dbToGain(params.band[b].gain)); smDrive[b].reset(smDrive[b].getTarget()); }
        smHeat.reset(params.heat); smLowX.reset(params.lowX); smHighX.reset(params.highX); smCrushMix.reset(smCrushMix.getTarget());
        reset();
    }
    void reset() noexcept
    {
        for (int c = 0; c < 2; ++c) { xLow[c].reset(); xHigh[c].reset(); apLow[c].reset(); lowCut[c].reset(); highCut[c].reset(); tiltEq[c].reset(); }
        for (int b = 0; b < 3; ++b) for (int ms = 0; ms < 2; ++ms) shaper[b][ms].reset();
    }
    void setParams(const CoreParams& p) noexcept
    {
        params = p;
        smLowX.setTarget(p.lowX); smHighX.setTarget(p.highX); smHeat.setTarget(p.heat);
        for (int c = 0; c < 2; ++c)
        {
            lowCut[c].setSlope(static_cast<CutFilter::Slope>(clampi(p.lowSlope, 0, 3)));
            highCut[c].setSlope(static_cast<CutFilter::Slope>(clampi(p.highSlope, 0, 3)));
            lowCut[c].setCutoff(p.lowCutHz); highCut[c].setCutoff(p.highCutHz); tiltEq[c].setTilt(p.tilt);
        }
        const bool anySolo = p.band[0].solo || p.band[1].solo || p.band[2].solo;
        for (int b = 0; b < 3; ++b)
        {
            smDrive[b].setTarget(clampf(p.band[b].drive * (0.25f + p.drive * 1.5f), 0.0f, 1.0f));
            character[b] = p.band[b].character;
            smBandMix[b].setTarget(p.band[b].enabled ? p.band[b].mix : 0.0f);
            const bool muted = anySolo && ! p.band[b].solo;
            smBandGain[b].setTarget(muted ? 0.0f : dbToGain(p.band[b].gain));
            if (p.vowel)
            {
                // TUNE (log 20 Hz..20 kHz -> 0..1) sweeps A - E - I - O - U; two formants per band
                static const float F1[5] = { 730.0f, 530.0f, 390.0f, 570.0f, 440.0f }, F2[5] = { 1090.0f, 1840.0f, 2290.0f, 840.0f, 1020.0f };
                const float t = clampf(std::log(p.band[b].tuneHz / 20.0f) / std::log(1000.0f), 0.0f, 1.0f) * 4.0f;
                const int i0 = clampi(int(t), 0, 3); const float f = t - float(i0);
                const float f1 = lerp(F1[i0], F1[i0 + 1], f), f2 = lerp(F2[i0], F2[i0 + 1], f);
                // bands below/above speech range get the formants transposed into them
                const float shift = b == 0 ? 0.25f : (b == 2 ? 3.0f : 1.0f);
                for (int ms = 0; ms < 2; ++ms) shaper[b][ms].setResonance(f1 * shift, p.band[b].res, f2 * shift);
            }
            else
            {
                // resonance centre clamped inside the band
                const float lo = b == 0 ? 20.0f : (b == 1 ? p.lowX : p.highX), hi = b == 0 ? p.lowX : (b == 1 ? p.highX : 20000.0f);
                const float hz = clampf(p.band[b].tuneHz, lo * 1.05f, hi * 0.95f);
                for (int ms = 0; ms < 2; ++ms) shaper[b][ms].setResonance(hz, p.band[b].res);
            }
        }
        smCrushMix.setTarget(p.crush.on ? p.crush.mix : 0.0f);
        crushQ = std::pow(2.0f, clampf(p.crush.bits, 2.0f, 16.0f) - 1.0f);
        crushStep = lerp(1.0f, 0.04f, clampf(p.crush.rate, 0.0f, 1.0f));
        smMix.setTarget(p.mix); smOut.setTarget(dbToGain(p.output)); smWidth.setTarget(p.width);
        smSub.setTarget(p.subLock ? 1.0f : 0.0f); smFocus.setTarget(p.focus); smPunch.setTarget(p.punch);
    }

    void process(float* L, float* R, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            const float mix = smMix.next(), out = smOut.next(), wid = smWidth.next(), sub = smSub.next(), focus = smFocus.next();
            // PUNCH: transient-ness of the input (fast env vs slow env) decides how much of each band bypasses the shaper
            const float punch = smPunch.next();
            {
                const float a = std::max(std::fabs(dryL), std::fabs(dryR));
                envFast += (a - envFast) * (a > envFast ? tFast : tSlow * 2.0f);
                envSlow += (a - envSlow) * tSlow;
                transient = clampf((envFast - envSlow) / (envSlow + 1e-3f), 0.0f, 1.0f);
            }
            const float satWeight = 1.0f - std::fabs(punch) * (punch < 0.0f ? transient : 1.0f - transient);
            // smoothed drive / heat / crossovers: coefficients refreshed every 16 samples
            const float heatNow = smHeat.next(), lx = smLowX.next(), hx = smHighX.next();
            for (int b = 0; b < 3; ++b) driveNow[b] = smDrive[b].next();
            if ((++coefCounter & 15) == 0)
            {
                for (int c = 0; c < 2; ++c) { xLow[c].setCutoff(lx); xHigh[c].setCutoff(hx); apLow[c].setCutoff(hx); }
                for (int b = 0; b < 3; ++b) for (int ms = 0; ms < 2; ++ms) shaper[b][ms].set(character[b], driveNow[b], heatNow);
            }
            // split
            float band[3][2];
            for (int c = 0; c < 2; ++c)
            {
                const float x = c == 0 ? dryL : dryR;
                float lo, rest, mid, hi;
                xLow[c].process(x, lo, rest);
                xHigh[c].process(rest, mid, hi);
                lo = apLow[c].allpass(lo);                 // phase-match the low band to the second crossover
                band[0][c] = lo; band[1][c] = mid; band[2][c] = hi;
            }
            float wetL = 0.0f, wetR = 0.0f;
            for (int b = 0; b < 3; ++b)
            {
                const float bm = smBandMix[b].next(), bg = smBandGain[b].next();
                float m = 0.5f * (band[b][0] + band[b][1]), s = 0.5f * (band[b][0] - band[b][1]);
                // focus: scale how much of mid / side goes through the shaper
                const float fm = focus > 0 ? 1.0f - focus : 1.0f, fsd = focus < 0 ? 1.0f + focus : 1.0f;
                float pm = shaper[b][0].process(m * fm) + m * (1.0f - fm);
                float ps = shaper[b][1].process(s * fsd) + s * (1.0f - fsd);
                pm = lerp(m, pm, satWeight); ps = lerp(s, ps, satWeight);
                float oL = pm + ps, oR = pm - ps;
                if (b == 0)   // sub lock: low band stays clean and mono
                {
                    const float mono = m;
                    oL = lerp(oL, mono, sub); oR = lerp(oR, mono, sub);
                }
                oL = lerp(band[b][0], oL, bm) * bg; oR = lerp(band[b][1], oR, bm) * bg;
                wetL += oL; wetR += oR;
            }
            // global bit crusher on the summed wet signal (sub-locked low band stays clean)
            {
                const float cm = smCrushMix.next();
                if (cm > 0.0005f)
                {
                    crushPhase += crushStep;
                    if (crushPhase >= 1.0f) { crushPhase -= 1.0f; crushHoldL = std::round(wetL * crushQ) / crushQ; crushHoldR = std::round(wetR * crushQ) / crushQ; }
                    wetL = lerp(wetL, crushHoldL, cm); wetR = lerp(wetR, crushHoldR, cm);
                }
            }
            // width
            {
                const float midv = 0.5f * (wetL + wetR), side = 0.5f * (wetL - wetR) * wid;
                wetL = midv + side; wetR = midv - side;
            }
            wetL = tiltEq[0].process(highCut[0].process(lowCut[0].process(wetL)));
            wetR = tiltEq[1].process(highCut[1].process(lowCut[1].process(wetR)));
            wetL = softCeiling(wetL); wetR = softCeiling(wetR);
            const float a = std::cos(mix * 1.5707963f), bgain = std::sin(mix * 1.5707963f);
            L[i] = (dryL * a + wetL * bgain) * out;
            R[i] = (dryR * a + wetR * bgain) * out;
            if (wetTapL) { wetTapL[i] = wetL * bgain * out; wetTapR[i] = wetR * bgain * out; }
        }
    }
    void processMono(float* M, int n) noexcept
    {
        for (int s0 = 0; s0 < n; s0 += 4096)
        {
            const int len = std::min(4096, n - s0);
            for (int i = 0; i < len; ++i) scratch[i] = M[s0 + i];
            process(M + s0, scratch, len);
            for (int i = 0; i < len; ++i) M[s0 + i] = 0.5f * (M[s0 + i] + scratch[i]);
        }
    }
    void setWetTap(float* l, float* r) noexcept { wetTapL = l; wetTapR = r; }
    float bandAutoGainDb(int b) const noexcept { return shaper[b][0].autoGainDb(); }

private:
    double fs = 48000.0;
    CoreParams params;
    CrossoverLR4 xLow[2], xHigh[2], apLow[2];
    BandShaper shaper[3][2];    // [band][mid/side]
    CutFilter lowCut[2], highCut[2]; TiltEQ tiltEq[2];
    Smoother smMix, smOut, smWidth, smSub, smFocus, smBandMix[3], smBandGain[3], smDrive[3], smHeat, smLowX, smHighX;
    int character[3] = { 0, 0, 0 }; float driveNow[3] = { 0, 0, 0 }; unsigned coefCounter = 0;
    Smoother smPunch; float tFast = 0.01f, tSlow = 0.001f, envFast = 0.0f, envSlow = 0.0f, transient = 0.0f;
    Smoother smCrushMix; float crushQ = 2048.0f, crushStep = 1.0f, crushPhase = 0.0f, crushHoldL = 0.0f, crushHoldR = 0.0f;
    float scratch[4096] {};
    float* wetTapL = nullptr; float* wetTapR = nullptr;
};

} // namespace zw
