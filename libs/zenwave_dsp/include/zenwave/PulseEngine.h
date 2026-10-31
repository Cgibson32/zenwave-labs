#pragma once
// PULSE: multiband dynamics for bass music.
//
//   in → IN gain → LR4 split (LOW | MID | HIGH)
//      → per band: adaptive OTT compressor (upward + downward, lookahead, program-dependent release,
//                  thresholds that follow the material) → band gain
//      → sum → dynamic EQ (4 floating nodes: compress or expand a frequency only when it misbehaves)
//      → transient shaper (ATTACK / SUSTAIN) → DUCK (pattern or external sidechain) → GRIT → mix → OUT
//
// Everything that reacts is smoothed; coefficient refreshes run every 16 samples.

#include "Utils.h"
#include "Filters.h"
#include "CoreEngine.h"      // CrossoverLR4
#include <cmath>
#include <array>
#include <vector>

namespace zw {

struct PulseBand
{
    bool  enabled = true, solo = false;
    float depth = 1.0f;      // 0..1 scales this band's compression
    float gain  = 0.0f;      // dB
    float attack = 1.0f;     // multiplier on the band's base attack
    float release = 1.0f;    // multiplier on the band's base release
};

struct PulseNode
{
    bool  on = false;
    float freqHz = 300.0f;   // 20..20k
    float q = 2.0f;          // 0.3..12
    float thresh = 0.0f;     // dB relative to the node's own long-term average
    float range = -6.0f;     // dB: negative = cut when the band gets loud (dynamic cut), positive = boost when loud
};

struct PulseParams
{
    float inGain = 0.0f;     // dB
    float depth = 0.5f;      // 0..1 master compression amount
    float time = 0.5f;       // 0..1 fast .. slow
    bool  autoMode = true;   // thresholds follow a slow loudness average
    bool  subLock = true;    // low band: no dynamics below the low crossover (gain still applies)
    bool  grit = false;      // soft tape stage after the dynamics
    float shapeAttack = 0.0f;  // -1..1 transient shaper
    float shapeSustain = 0.0f; // -1..1
    float duck = 0.0f;       // 0..1 manual / pattern duck amount
    float scAmount = 0.0f;   // 0..1 external sidechain duck amount
    float scReleaseMs = 150.0f;
    float lowX = 150.0f, highX = 3000.0f;
    float mix = 1.0f, output = 0.0f;
    PulseBand band[3];
    PulseNode node[4];
};

// One band's upward/downward compressor with adaptive thresholds and program-dependent release
class OttBand
{
public:
    void prepare(double sr, float baseAttackMs, float baseReleaseMs)
    {
        fs = sr; baseA = baseAttackMs; baseR = baseReleaseMs;
        env = 1e-4f; slowAvg = -30.0f; dens = 0.0f; grDb = 0.0f;
        slowK = 1.0f - std::exp(-1.0f / (6.0f * float(sr)));
        densFastK = 1.0f - std::exp(-1.0f / (0.01f * float(sr))); densSlowK = 1.0f - std::exp(-1.0f / (0.4f * float(sr)));
        densFast = densSlow = 1e-4f;
        depthK = 1.0f - std::exp(-1.0f / (0.025f * float(sr))); depth = depthTarget;
    }
    void set(float depthAmt, float timeAmt, float atkMul, float relMul, bool adaptive)
    {
        depthTarget = depthAmt; adapt = adaptive;
        const float t = 0.3f + timeAmt * 1.7f;      // 0.3x .. 2x
        const float aMs = baseA * t * atkMul, rMs = baseR * t * relMul;
        aK = 1.0f - std::exp(-1.0f / (std::max(0.05f, aMs) * 0.001f * float(fs)));
        rKbase = rMs;
    }
    // returns linear gain for this sample; x is the (undelayed) detector input
    float process(float detL, float detR)
    {
        const float a = std::max(std::fabs(detL), std::fabs(detR));
        // density: how continuous the material is (fast env vs slow env)
        densFast += (a - densFast) * densFastK; densSlow += (a - densSlow) * densSlowK;
        dens = clampf(densFast / (densSlow + 1e-5f), 0.0f, 2.0f);
        // program-dependent release: dense material releases faster
        const float rMs = rKbase * lerp(1.3f, 0.45f, clampf(dens * 0.5f, 0.0f, 1.0f));
        const float rK = 1.0f - std::exp(-1.0f / (std::max(1.0f, rMs) * 0.001f * float(fs)));
        env += (a - env) * (a > env ? aK : rK);
        const float lvl = 20.0f * std::log10(env + 1e-6f);
        if (adapt && a > 1e-4f) slowAvg += (lvl - slowAvg) * slowK;
        const float ref = adapt ? slowAvg : -18.0f;
        // OTT law: above ref+2 compress 3:1, below ref-2 expand upward 3:1 (bounded to +18 dB)
        float g = 0.0f;
        const float over = lvl - (ref + 2.0f), under = (ref - 2.0f) - lvl;
        if (over > 0.0f) g = -over * 0.67f;
        else if (under > 0.0f) g = std::min(under * 0.67f, 18.0f);
        depth += (depthTarget - depth) * depthK;                  // depth glides (~25 ms) so modulated depth never steps the gain
        g *= depth;
        grDb += (g - grDb) * 0.02f;                               // gain path smoothing (anti-zipper)
        return dbToGain(grDb);
    }
    float gainReductionDb() const { return grDb; }
    float density() const { return dens; }
private:
    double fs = 48000.0; float baseA = 10.0f, baseR = 100.0f, aK = 0.01f, rKbase = 100.0f, depth = 1.0f, depthTarget = 1.0f, depthK = 0.001f; bool adapt = true;
    float env = 0, slowAvg = -30.0f, slowK = 0.00001f, dens = 0, densFast = 0, densSlow = 0, densFastK = 0.01f, densSlowK = 0.001f, grDb = 0.0f;
};

// Dynamic EQ node: peaking biquad whose gain follows a band-limited detector against its own average
class DynNode
{
public:
    void prepare(double sr) { fs = sr; for (auto& c : ch) c = {}; det.setSampleRate(sr); det.setMode(SVF::Mode::Bandpass); env = 1e-4f; avg = -40.0f; gDb = 0.0f; avgK = 1.0f - std::exp(-1.0f / (3.0f * float(sr))); }
    void set(const PulseNode& n)
    {
        on = n.on; range = n.range; thresh = n.thresh;
        freq = clampf(n.freqHz, 20.0f, float(fs * 0.45)); q = clampf(n.q, 0.3f, 12.0f);
        det.setCutoff(freq); det.setResonance(std::max(0.7f, q));
    }
    // detector from the mono sum; returns current gain in dB (for UI)
    void detect(float mono)
    {
        if (! on) { gDb += (0.0f - gDb) * 0.01f; return; }
        const float b = std::fabs(det.process(mono));
        const float aK = 0.0052f, rK = 0.0003f;   // ~4 ms / ~70 ms at 48k
        env += (b - env) * (b > env ? aK : rK);
        const float lvl = 20.0f * std::log10(env + 1e-6f);
        if (b > 1e-4f) avg += (lvl - avg) * avgK;
        const float over = lvl - (avg + thresh);
        const float target = range * clampf(over / 12.0f, 0.0f, 1.0f);   // full range 12 dB past threshold
        gDb += (target - gDb) * 0.01f;
    }
    void refresh()   // every 16 samples: RBJ peaking coefficients for the current gain
    {
        const float A = std::pow(10.0f, gDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / float(fs), alpha = std::sin(w0) / (2.0f * q);
        const float cw = std::cos(w0);
        const float b0 = 1 + alpha * A, b1 = -2 * cw, b2 = 1 - alpha * A, a0 = 1 + alpha / A, a1 = -2 * cw, a2 = 1 - alpha / A;
        c0 = b0 / a0; c1 = b1 / a0; c2 = b2 / a0; d1 = a1 / a0; d2 = a2 / a0;
    }
    float process(int c, float x)
    {
        if (! on && std::fabs(gDb) < 0.01f) return x;
        auto& s = ch[c];
        const float y = c0 * x + c1 * s.x1 + c2 * s.x2 - d1 * s.y1 - d2 * s.y2;
        s.x2 = s.x1; s.x1 = x; s.y2 = s.y1; s.y1 = y;
        return y;
    }
    float gainDb() const { return gDb; }
private:
    struct S { float x1 = 0, x2 = 0, y1 = 0, y2 = 0; } ch[2];
    SVF det; double fs = 48000.0; bool on = false;
    float freq = 300.0f, q = 2.0f, thresh = 0.0f, range = -6.0f, env = 0, avg = -40.0f, avgK = 0.0001f, gDb = 0;
    float c0 = 1, c1 = 0, c2 = 0, d1 = 0, d2 = 0;
};

class PulseEngine
{
public:
    static constexpr int kLookahead = 144;   // 3 ms at 48k (scaled in prepare)

    void prepare(double sr, int /*maxBlock*/)
    {
        fs = sr; la = int(sr * 0.003);
        for (int c = 0; c < 2; ++c)
        {
            xLow[c].setSampleRate(sr); xHigh[c].setSampleRate(sr); apLow[c].setSampleRate(sr);
            for (int b = 0; b < 3; ++b) { laBuf[b][c].assign(size_t(la + 8), 0.0f); }
            scLP[c].setSampleRate(sr); scLP[c].setLowpass(200.0f);
            gritDC[c].setSampleRate(sr); gritDC[c].setHighpass(10.0f);
        }
        laPos = 0; dryPos = 0;
        for (int c = 0; c < 2; ++c) dryLa[c].assign(size_t(la + 8), 0.0f);
        comp[0].prepare(sr, 12.0f, 90.0f); comp[1].prepare(sr, 4.0f, 45.0f); comp[2].prepare(sr, 1.0f, 22.0f);
        for (auto& n : node) n.prepare(sr);
        for (auto* s : { &smIn, &smOut, &smMix, &smDuck, &smSc, &smLowX, &smHighX, &smAtk, &smSus, &smGrit, &smSub }) { s->setSampleRate(sr); s->setTime(30.0f); }
        for (auto& s : smBandGain) { s.setSampleRate(sr); s.setTime(30.0f); }
        tFastK = 1.0f - std::exp(-1.0f / (0.0012f * float(sr))); tSlowK = 1.0f - std::exp(-1.0f / (0.05f * float(sr)));
        scEnv = 0.0f; tFast = tSlow = 0.0f;
        setParams(params);
        smIn.reset(dbToGain(params.inGain)); smOut.reset(dbToGain(params.output)); smMix.reset(params.mix); smDuck.reset(params.duck); smSc.reset(params.scAmount);
        smLowX.reset(params.lowX); smHighX.reset(params.highX); smAtk.reset(params.shapeAttack); smSus.reset(params.shapeSustain); smGrit.reset(params.grit ? 1.0f : 0.0f); smSub.reset(params.subLock ? 1.0f : 0.0f);
        for (int b = 0; b < 3; ++b) smBandGain[b].reset(smBandGain[b].getTarget());
        reset();
    }
    void reset()
    {
        for (int c = 0; c < 2; ++c) { xLow[c].reset(); xHigh[c].reset(); apLow[c].reset(); scLP[c].reset(); gritDC[c].reset(); for (int b = 0; b < 3; ++b) std::fill(laBuf[b][c].begin(), laBuf[b][c].end(), 0.0f); }
    }
    void setParams(const PulseParams& p)
    {
        params = p;
        smIn.setTarget(dbToGain(p.inGain)); smOut.setTarget(dbToGain(p.output)); smMix.setTarget(p.mix);
        smDuck.setTarget(clampf(p.duck, 0.0f, 1.0f)); smSc.setTarget(p.scAmount);
        smLowX.setTarget(p.lowX); smHighX.setTarget(std::max(p.highX, p.lowX * 2.5f));
        smAtk.setTarget(p.shapeAttack); smSus.setTarget(p.shapeSustain); smGrit.setTarget(p.grit ? 1.0f : 0.0f); smSub.setTarget(p.subLock ? 1.0f : 0.0f);
        const bool anySolo = p.band[0].solo || p.band[1].solo || p.band[2].solo;
        for (int b = 0; b < 3; ++b)
        {
            comp[b].set(p.band[b].enabled ? p.depth * p.band[b].depth : 0.0f, p.time, p.band[b].attack, p.band[b].release, p.autoMode);
            const bool muted = anySolo && ! p.band[b].solo;
            smBandGain[b].setTarget(muted ? 0.0f : dbToGain(p.band[b].gain));
        }
        for (int i = 0; i < 4; ++i) node[i].set(p.node[i]);
        scRelK = 1.0f - std::exp(-1.0f / (std::max(5.0f, p.scReleaseMs) * 0.001f * float(fs)));
    }

    // scL/scR may be null (no external sidechain)
    void process(float* L, float* R, const float* scL, const float* scR, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const float ing = smIn.next(), outg = smOut.next(), mix = smMix.next(), duck = smDuck.next(), scAmt = smSc.next();
            const float lx = smLowX.next(), hx = smHighX.next(), atk = smAtk.next(), sus = smSus.next(), grit = smGrit.next(), sub = smSub.next();
            if ((++coefCounter & 15) == 0)
            {
                for (int c = 0; c < 2; ++c) { xLow[c].setCutoff(lx); xHigh[c].setCutoff(hx); apLow[c].setCutoff(hx); }
                for (auto& nd : node) nd.refresh();
            }
            const float dryL = L[i], dryR = R[i];
            const float xL = dryL * ing, xR = dryR * ing;

            // split
            float band[3][2];
            for (int c = 0; c < 2; ++c)
            {
                const float x = c == 0 ? xL : xR;
                float lo, rest, mid, hi;
                xLow[c].process(x, lo, rest); xHigh[c].process(rest, mid, hi);
                band[0][c] = apLow[c].allpass(lo); band[1][c] = mid; band[2][c] = hi;
            }
            // per band: detector on the undelayed band, gain applied to the lookahead-delayed band
            float wetL = 0.0f, wetR = 0.0f;
            for (int b = 0; b < 3; ++b)
            {
                float g = comp[b].process(band[b][0], band[b][1]);
                if (b == 0) g = lerp(g, 1.0f, sub);                 // sub lock: no dynamics on the low band
                const float bg = smBandGain[b].next();
                float dL = laBuf[b][0][size_t(laPos)], dR = laBuf[b][1][size_t(laPos)];
                laBuf[b][0][size_t(laPos)] = band[b][0]; laBuf[b][1][size_t(laPos)] = band[b][1];
                wetL += dL * g * bg; wetR += dR * g * bg;
                grVis[b] = comp[b].gainReductionDb();
            }
            laPos = (laPos + 1) % la;

            // dynamic EQ nodes
            {
                const float mono = 0.5f * (wetL + wetR);
                for (auto& nd : node) { nd.detect(mono); wetL = nd.process(0, wetL); wetR = nd.process(1, wetR); }
            }
            // transient shaper: fast/slow envelope difference → attack and sustain gains
            {
                const float a = std::max(std::fabs(wetL), std::fabs(wetR));
                tFast += (a - tFast) * tFastK; tSlow += (a - tSlow) * tSlowK;
                const float tr = clampf((tFast - tSlow) / (tSlow + 1e-3f), 0.0f, 1.0f);   // 1 = in a transient
                const float gA = dbToGain(atk * 9.0f * tr);                                  // ±9 dB on attacks
                const float gS = dbToGain(sus * 9.0f * (1.0f - tr) * clampf(tSlow * 4.0f, 0.0f, 1.0f));
                transientVis = tr;
                wetL *= gA * gS; wetR *= gA * gS;
            }
            // duck: manual/pattern amount + external sidechain
            {
                float scLevel = 0.0f;
                if (scL) { const float s0 = std::max(std::fabs(scLP[0].process(scL[i])), std::fabs(scLP[1].process(scR ? scR[i] : scL[i]))); scLevel = s0; }
                scEnv += (scLevel - scEnv) * (scLevel > scEnv ? 0.3f : scRelK);
                const float scDuck = clampf(scEnv * 6.0f, 0.0f, 1.0f) * scAmt;
                const float totalDuck = clampf(duck + scDuck, 0.0f, 1.0f);
                const float dg = 1.0f - totalDuck;
                duckVis = totalDuck;
                wetL *= dg; wetR *= dg;
            }
            // grit: gentle tape stage
            if (grit > 0.001f)
            {
                const float gl = gritDC[0].process(fastTanh(wetL * 1.6f) / 1.6f * 1.15f), gr = gritDC[1].process(fastTanh(wetR * 1.6f) / 1.6f * 1.15f);
                wetL = lerp(wetL, gl, grit); wetR = lerp(wetR, gr, grit);
            }
            wetL = softCeiling(wetL); wetR = softCeiling(wetR);
            // dry is delayed by the same lookahead so mix stays phase-coherent
            const float ddL = dryLa[0][size_t(dryPos)], ddR = dryLa[1][size_t(dryPos)];
            dryLa[0][size_t(dryPos)] = dryL; dryLa[1][size_t(dryPos)] = dryR; dryPos = (dryPos + 1) % la;
            const float a = std::cos(mix * 1.5707963f), bgn = std::sin(mix * 1.5707963f);
            L[i] = (ddL * a + wetL * bgn) * outg; R[i] = (ddR * a + wetR * bgn) * outg;
            if (wetTapL) { wetTapL[i] = wetL * bgn * outg; wetTapR[i] = wetR * bgn * outg; }
        }
    }
    void processMono(float* M, int n)
    {
        for (int s0 = 0; s0 < n; s0 += 4096)
        {
            const int len = std::min(4096, n - s0);
            for (int i = 0; i < len; ++i) scratch[i] = M[s0 + i];
            process(M + s0, scratch, nullptr, nullptr, len);
            for (int i = 0; i < len; ++i) M[s0 + i] = 0.5f * (M[s0 + i] + scratch[i]);
        }
    }
    int latencySamples() const { return la; }
    void setWetTap(float* l, float* r) noexcept { wetTapL = l; wetTapR = r; }

    float grVis[3] = { 0, 0, 0 }; float transientVis = 0.0f, duckVis = 0.0f;
    float nodeGainDb(int i) const { return node[i].gainDb(); }
    float density(int b) const { return comp[b].density(); }

private:
    double fs = 48000.0; int la = 144, laPos = 0, dryPos = 0; unsigned coefCounter = 0;
    PulseParams params;
    CrossoverLR4 xLow[2], xHigh[2], apLow[2];
    OttBand comp[3];
    std::vector<float> laBuf[3][2], dryLa[2];
    DynNode node[4];
    OnePole scLP[2], gritDC[2];
    Smoother smIn, smOut, smMix, smDuck, smSc, smLowX, smHighX, smAtk, smSus, smGrit, smSub, smBandGain[3];
    float tFast = 0, tSlow = 0, tFastK = 0.01f, tSlowK = 0.001f, scEnv = 0, scRelK = 0.001f;
    float scratch[4096] {};
    float* wetTapL = nullptr; float* wetTapR = nullptr;
};

} // namespace zw
