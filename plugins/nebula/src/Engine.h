#pragma once
// NEBULA engine: planetary wavetables + harmonic carving + per-voice filter/drive + space.
//
// Tables are generated procedurally from harmonic recipes (no sample data), rendered by IFFT into
// 32 frames x 6 band-limited mip levels. CARVE is a 32-band per-harmonic gain curve applied to the
// recipe before rendering, so carving changes the wave itself. Re-rendering a table takes ~2 ms and
// happens on the message thread; the audio thread swaps in the new table atomically.

#include <JuceHeader.h>
#include <zenwave/zenwave.h>
#include <zenwave/PulseEngine.h>   // OttBand for GLUE
#include <array>
#include <atomic>
#include <memory>
#include <cmath>
#include <complex>
#include "Tables.h"
#include "SampledTables.h"

namespace nebula {

constexpr int kProceduralTables = 49;
constexpr int kFrameLen = 1024, kFrames = 32, kMips = 6, kPartials = 384, kCarveBands = 8, kTables = kProceduralTables + kSampledTableCount;

inline const juce::StringArray& tableNames()
{
    static const juce::StringArray n = [] { juce::StringArray a(allTableNames()); for (int i = 0; i < kSampledTableCount; ++i) a.add(kSampledTables[i].name); return a; }();
    return n;
}
inline const std::vector<std::pair<int, const char*>>& allTableSections()
{
    static const std::vector<std::pair<int, const char*>> s = [] { auto v = tableSections(); v.push_back({ kProceduralTables, "REAL VOICE" }); return v; }();
    return s;
}

// Sampled table: harmonic spectrum per frame measured from a real recording at refHz. Re-sampled at the played
// pitch so the singer's formants stay fixed in Hz; frames interpolate so POSITION replays the take's own motion.
inline void sampledRecipe(const SampledTable& st, float pos, float* amp, float* phase, float f0Hint)
{
    const float f0 = f0Hint > 0.0f ? f0Hint : st.refHz;
    const float fp = juce::jlimit(0.0f, 1.0f, pos) * (kSampledFrames - 1); const int fa = int(fp), fb = std::min(kSampledFrames - 1, fa + 1); const float ff = fp - fa;
    auto dbAt = [&](int frame, float x)   // x: fractional harmonic index (1-based) in the stored spectrum
    {
        if (x >= float(kSampledPartials)) return -200.0f;
        x = std::max(1.0f, x) - 1.0f; const int i = int(x); const int j = std::min(kSampledPartials - 1, i + 1); const float t = x - i;
        const float a = (st.db[frame][i] - 255.0f) * 0.5f, b = (st.db[frame][j] - 255.0f) * 0.5f;
        return a + (b - a) * t;
    };
    for (int h = 1; h <= kPartials; ++h)
    {
        const float x = h * f0 / st.refHz;
        const float db = dbAt(fa, x) * (1.0f - ff) + dbAt(fb, x) * ff;
        amp[h - 1] = db < -120.0f ? 0.0f : std::pow(10.0f, db / 20.0f);
        phase[h - 1] = fixedPhase(h, 777);
    }
}

// Harmonic recipe: amplitude + phase per partial for a given table and frame position (0..1)
// f0Hint: the pitch (Hz) this frame will be played at, so formant-type tables keep their formants fixed in Hz. 0 = nominal.
inline void recipe(int table, float pos, float* amp, float* phase, juce::Random& rng, float f0Hint = 0.0f)
{
    for (int h = 0; h < kPartials; ++h) { amp[h] = 0.0f; phase[h] = 0.0f; }
    auto set = [&](int h, float a, float ph = 0.0f) { if (h >= 1 && h <= kPartials) { amp[h - 1] = a; phase[h - 1] = ph; } };
    const float t = pos;
    if (table >= kProceduralTables) { sampledRecipe(kSampledTables[juce::jlimit(0, kSampledTableCount - 1, table - kProceduralTables)], t, amp, phase, f0Hint); return; }
    if (table >= kLegacyTables) { recipeEx(table, t, amp, phase, rng, f0Hint); return; }
    switch (table)
    {
        case 0: // Glass: pure odd/even bell partials that migrate upward; sine -> glassy chime
            for (int h = 1; h <= 64; ++h)
            {
                const float even = (h % 2 == 0) ? 1.0f : 0.0f;
                const float a = (h == 1 ? 1.0f : 0.0f) * (1.0f - t * 0.5f)
                              + t * std::exp(-0.09f * h) * (0.35f + 0.65f * even) * (1.0f + 0.5f * std::sin(h * 0.7f + t * 6.0f));
                set(h, a, (h % 3) * 1.1f);
            }
            break;
        case 1: // Voice: formant bumps (two formants) moving through vowels on a pulse
            for (int h = 1; h <= 96; ++h)
            {
                const float f = h * (f0Hint > 0.0f ? f0Hint : 110.0f);
                const float F1 = 300.0f + 500.0f * t, F2 = 2400.0f - 1400.0f * t;
                const float form = std::exp(-std::pow((f - F1) / 150.0f, 2.0f)) + 0.6f * std::exp(-std::pow((f - F2) / 300.0f, 2.0f));
                set(h, (0.08f + form) / std::pow(float(h), 0.6f));
            }
            break;
        case 2: // Metal: inharmonic-feeling clusters (stretched partial spacing) sharpening with position
            for (int k = 1; k <= 40; ++k)
            {
                const int h = int(std::round(k * (1.0f + 0.18f * t * k * 0.1f)));
                set(h, std::exp(-0.05f * k) * (k % 2 ? 1.0f : 0.5f + 0.5f * t), rng.nextFloat() * 6.28f);
            }
            break;
        case 3: // Grain: dense noise-like spectrum whose brightness and texture move
            for (int h = 1; h <= 200; ++h)
            {
                const float r = rng.nextFloat();
                set(h, (0.4f + 0.6f * r) * std::exp(-h * (0.03f - 0.02f * t)) * (h == 1 ? 2.0f : 1.0f), rng.nextFloat() * 6.28f);
            }
            break;
        case 4: // Dune: saw -> square -> pulse-width morph, the classic analogue sweep
            for (int h = 1; h <= 128; ++h)
            {
                const float saw = 1.0f / h, sq = (h % 2) ? 1.0f / h : 0.0f;
                const float pw = 0.5f - 0.4f * std::max(0.0f, t * 2.0f - 1.0f);
                const float pulse = std::fabs(std::sin(3.14159f * h * pw)) / h;
                const float a = t < 0.5f ? zw::lerp(saw, sq, t * 2.0f) : pulse;
                set(h, a);
            }
            break;
        case 5: // Halo: a fundamental with a shimmering cluster an octave + fifth up that blooms
            for (int h = 1; h <= 48; ++h)
            {
                float a = h == 1 ? 1.0f : 0.0f;
                if (h == 2) a = 0.5f; if (h == 3) a = 0.35f * t; if (h == 6) a = 0.3f * t; if (h == 12) a = 0.25f * t * t;
                a += 0.15f * t * std::exp(-0.1f * h) * (1.0f + std::sin(h * 1.9f + t * 9.0f));
                set(h, a, h * 0.4f);
            }
            break;
        case 6: // Ion: hard bright buzz, high partials emphasised, comb-like holes that slide
            for (int h = 1; h <= 160; ++h)
            {
                const float comb = 0.5f + 0.5f * std::cos(h * (0.3f + 1.5f * t));
                set(h, (0.3f + 0.7f * comb) / std::pow(float(h), 0.35f));
            }
            break;
        default: // Void: sub-heavy, slow-moving dark hum; upper partials only arrive at the top of the position
            for (int h = 1; h <= 24; ++h)
                set(h, h == 1 ? 1.0f : (h == 2 ? 0.4f : std::max(0.0f, t * 1.5f - 0.5f) * std::exp(-0.25f * h)));
            break;
    }
}

// A hand-placed carve band: centre harmonic (1..kPartials, log), width in octaves, gain (linear; 0 = fully cut)
struct CarveBand { bool on = false; float centre = 4.0f, widthOct = 0.5f, gain = 0.0f; };
inline bool operator==(const CarveBand& a, const CarveBand& b) { return a.on == b.on && a.centre == b.centre && a.widthOct == b.widthOct && a.gain == b.gain; }
inline bool operator!=(const CarveBand& a, const CarveBand& b) { return ! (a == b); }
inline float carveGainAt(const std::array<CarveBand, kCarveBands>& bands, int harmonic)
{
    float g = 1.0f; const float lh = std::log2(float(harmonic));
    for (const auto& b : bands)
    {
        if (! b.on) continue;
        const float d = (lh - std::log2(std::max(1.0f, b.centre))) / std::max(0.05f, b.widthOct * 0.5f);
        const float bell = std::exp(-d * d * 1.6f);                      // raised bell, ~width at half height
        g *= 1.0f + (b.gain - 1.0f) * bell;                               // 1 at the edges, gain at the centre
    }
    return std::max(0.0f, g);
}

struct Table
{
    // mip[m][frame][sample]; mip m allows partials up to kPartials >> m
    std::vector<float> data;   // kMips * kFrames * (kFrameLen + 1)   (+1 wraps for interpolation)
    const float* frame(int mip, int f) const { return data.data() + size_t((mip * kFrames + f) * (kFrameLen + 1)); }
};

// Renders one table with a carve curve. Not realtime-safe (FFT + allocation); call from the message thread.
// Mip bands: mip m serves fundamentals in (46.875 * 2^(m-1), 46.875 * 2^m] Hz (band-limited for 48 kHz); centre is the geometric middle.
constexpr float kMipBaseHz = 46.875f;
inline float mipCentreHz(int m) { return kMipBaseHz * std::pow(2.0f, float(m) - 0.5f); }

class TableRenderer
{
public:
    TableRenderer() : fft(10) {}
    std::shared_ptr<Table> render(int table, const std::array<CarveBand, kCarveBands>& carve, bool carveOn)
    {
        auto out = std::make_shared<Table>();
        out->data.assign(size_t(kMips * kFrames * (kFrameLen + 1)), 0.0f);
        std::vector<float> amp(kPartials), phase(kPartials);
        std::vector<juce::dsp::Complex<float>> spec(kFrameLen), time(kFrameLen);
        for (int f = 0; f < kFrames; ++f)
        {
            for (int m = 0; m < kMips; ++m)
            {
                juce::Random rng(1234 + table * 99 + f);   // deterministic per frame (same seed for every mip)
                recipe(table, float(f) / (kFrames - 1), amp.data(), phase.data(), rng, mipCentreHz(m));
                if (carveOn) for (int h = 0; h < kPartials; ++h) amp[size_t(h)] *= carveGainAt(carve, h + 1);
                const int maxH = juce::jmin(kPartials, (kFrameLen / 2 - 1) >> m);   // band limit per octave
                std::fill(spec.begin(), spec.end(), juce::dsp::Complex<float>(0, 0));
                for (int h = 1; h <= maxH; ++h)
                {
                    const float a = amp[size_t(h - 1)], ph = phase[size_t(h - 1)];
                    spec[size_t(h)] = juce::dsp::Complex<float>(a * std::cos(ph), a * std::sin(ph));
                    spec[size_t(kFrameLen - h)] = std::conj(spec[size_t(h)]);
                }
                fft.perform(spec.data(), time.data(), true);
                float* dst = out->data.data() + size_t((m * kFrames + f) * (kFrameLen + 1));
                float pk = 1e-6f, sq = 0.0f; for (int i = 0; i < kFrameLen; ++i) { const float v = time[size_t(i)].real(); pk = std::max(pk, std::fabs(v)); sq += v * v; }
                const float rms = std::sqrt(sq / kFrameLen) + 1e-6f;
                const float gain = std::min(0.9f / pk, 0.28f / rms);        // frames match in loudness, never clip
                for (int i = 0; i < kFrameLen; ++i) dst[i] = time[size_t(i)].real() * gain;
                dst[kFrameLen] = dst[0];
            }
        }
        return out;
    }
private:
    juce::dsp::FFT fft;
};

struct VoiceParams
{
    // per oscillator
    struct Osc { int table = 0; float position = 0.0f, level = 0.8f; int octave = 0, semis = 0; float fine = 0.0f; int unison = 1; float spread = 0.2f, width = 0.6f; bool on = true; } osc[2];
    float drift = 0.2f;          // per-voice slow pitch wander
    float filterHz = 8000.0f, res = 0.3f; int filterMode = 0; float envAmt = 0.0f, keyTrack = 0.5f;
    float drive = 0.0f;
    float ampA = 10.0f, ampD = 200.0f, ampS = 0.8f, ampR = 400.0f;   // ms / level
    float modA = 5.0f, modD = 300.0f, modS = 0.3f, modR = 300.0f;
    float glideMs = 0.0f;
    float lfoToPosition = 0.0f;  // movement macro: LFO1 -> position
    float vibrato = 0.0f;        // human pitch motion: depth, 5-6 Hz, eases in after the attack, independent per voice
    float breath = 0.0f;         // breath noise layer shaped like a voice (formant bandpasses), follows the amp envelope
    float evolve = 0.0f;         // per-voice slow motion: each note drifts its own position, filter, pan and level on its own clocks
};

class Adsr
{
public:
    void prepare(double sr) { fs = sr; }
    void set(float aMs, float dMs, float s, float rMs) { aK = coef(aMs); dK = coef(dMs); sus = s; rK = coef(rMs); }
    void noteOn() { stage = 1; }
    void noteOff() { if (stage != 0) stage = 4; }
    bool active() const { return stage != 0; }
    float next()
    {
        switch (stage)
        {
            case 1: v += (1.1f - v) * aK; if (v >= 1.0f) { v = 1.0f; stage = 2; } break;
            case 2: v += (sus - v) * dK; if (std::fabs(v - sus) < 0.001f) stage = 3; break;
            case 3: v = sus; break;
            case 4: v += (0.0f - v) * rK; if (v < 0.0005f) { v = 0.0f; stage = 0; } break;
            default: v = 0.0f; break;
        }
        return v;
    }
    float value() const { return v; }
private:
    float coef(float ms) const { return 1.0f - std::exp(-1.0f / (std::max(0.1f, ms) * 0.001f * float(fs))); }
    double fs = 48000.0; float aK = 0.01f, dK = 0.001f, sus = 0.8f, rK = 0.001f, v = 0.0f; int stage = 0;
};

class Voice
{
public:
    void prepare(double sr)
    {
        fs = sr; amp.prepare(sr); mod.prepare(sr);
        for (auto& f : filt) { f.setSampleRate(sr); f.setResonance(0.7f); }
        rng.setSeedRandomly();
        driftK = 1.0f - std::exp(-1.0f / (0.6f * float(sr)));
        posK = 1.0f - std::exp(-1.0f / (0.006f * float(sr)));
        for (int c = 0; c < 2; ++c) { for (auto& b : breathBp[c]) { b.setSampleRate(sr); b.setMode(zw::SVF::Mode::Bandpass); b.setResonance(2.2f); } breathLp[c].setSampleRate(sr); breathLp[c].setLowpass(7000.0f); }
        vibOnsetK = 1.0f - std::exp(-1.0f / (0.9f * float(sr)));
    }
    void noteOn(int n, float vel, float glideFrom, float glideMs)
    {
        note = n; velocity = vel; age = 0;
        targetPitch = float(n);
        pitch = glideMs > 0.5f && glideFrom > 0.0f ? glideFrom : targetPitch;
        glideK = glideMs > 0.5f ? 1.0f - std::exp(-1.0f / (glideMs * 0.001f * float(fs))) : 1.0f;
        const bool silent = amp.value() < 0.001f;
        for (int o = 0; o < 2; ++o) for (int u = 0; u < 8; ++u) { if (silent) ph[o][u] = rng.nextFloat(); driftState[o][u] = 0.0f; driftTarget[o][u] = rng.nextFloat() * 2.0f - 1.0f; }
        amp.noteOn(); mod.noteOn(); released = false;
        if (silent) { for (auto& f : filt) f.reset(); posSm[0] = posSm[1] = -1.0f; vibAmt = 0.0f; vibPhase = rng.nextFloat(); }
        vibRate = 4.6f + rng.nextFloat() * 1.3f;                                    // every singer has their own vibrato
        for (int k = 0; k < 4; ++k) { evRate[k] = (0.025f + rng.nextFloat() * 0.09f) / float(fs); if (silent) evPhase[k] = rng.nextFloat(); }
        breathF[0] = 900.0f + rng.nextFloat() * 300.0f; breathF[1] = 2300.0f + rng.nextFloat() * 600.0f;
    }
    void noteOff() { amp.noteOff(); mod.noteOff(); released = true; }
    bool active() const { return amp.active(); }
    int noteNumber() const { return note; }
    bool isReleased() const { return released; }
    int ageSamples() const { return age; }

    void setParams(const VoiceParams& p) { params = &p; amp.set(p.ampA, p.ampD, p.ampS, p.ampR); mod.set(p.modA, p.modD, p.modS, p.modR); }
    void setTables(const std::shared_ptr<Table>* t) { tables = t; }

    // Renders n samples, ADDING to L/R. lfoPos: current LFO1 value (-1..1) for the movement macro. bendSemis: pitch wheel.
    void render(float* L, float* R, int n, float lfoPos, float bendSemis, float pressure)
    {
        if (! params || ! tables) return;
        const auto& p = *params;
        for (int i = 0; i < n; ++i)
        {
            pitch += (targetPitch - pitch) * glideK;
            const float a = amp.next(), m = mod.next();
            if (! amp.active()) return;
            // filter: cutoff follows env, key tracking and pressure
            if ((age & 15) == 0)
            {
                const float kt = (pitch - 60.0f) / 12.0f * p.keyTrack;
                float hz = p.filterHz * std::pow(2.0f, kt + p.envAmt * 6.0f * m + pressure * 2.0f + ev[2] * 0.7f);
                hz = zw::clampf(hz, 20.0f, float(fs * 0.45));
                for (auto& f : filt) { f.setCutoff(hz); f.setResonance(0.5f + p.res * 9.5f); f.setMode(p.filterMode == 0 ? zw::SVF::Mode::Lowpass : p.filterMode == 1 ? zw::SVF::Mode::Highpass : zw::SVF::Mode::Bandpass); }
            }
            // evolve: four slow sines per voice (position A, position B, filter, pan) at rates no two notes share
            if (p.evolve > 0.0005f && (age & 31) == 0)
                for (int k = 0; k < 4; ++k) { evPhase[k] += evRate[k] * 32.0f; if (evPhase[k] >= 1.0f) evPhase[k] -= 1.0f; ev[k] = std::sin(evPhase[k] * 6.2831853f) * p.evolve; }
            else if (p.evolve <= 0.0005f) { ev[0] = ev[1] = ev[2] = ev[3] = 0.0f; }
            // human vibrato: eases in after the attack, a touch of rate wobble so it never sounds like an LFO
            float vib = 0.0f;
            if (p.vibrato > 0.0005f)
            {
                vibAmt += (1.0f - vibAmt) * vibOnsetK;
                vibPhase += (vibRate * (1.0f + 0.08f * driftState[0][0])) / float(fs); if (vibPhase >= 1.0f) vibPhase -= 1.0f;
                vib = std::sin(vibPhase * 6.2831853f) * p.vibrato * 0.45f * vibAmt;
            }
            float sL = 0.0f, sR = 0.0f;
            for (int o = 0; o < 2; ++o)
            {
                const auto& osc = p.osc[o];
                if (! osc.on || osc.level <= 0.0005f) continue;
                const Table* tb = tables[o].get(); if (! tb) continue;
                const float basePitch = pitch + osc.octave * 12.0f + osc.semis + osc.fine * 0.01f + bendSemis + vib;
                const int uni = juce::jlimit(1, 8, osc.unison);
                const float posTarget = zw::clampf(osc.position + p.lfoToPosition * 0.5f * lfoPos + ev[o] * 0.18f, 0.0f, 1.0f);
                if (posSm[o] < 0.0f) posSm[o] = posTarget;
                posSm[o] += (posTarget - posSm[o]) * posK;      // ~6 ms glide: modulated POSITION morphs, never steps
                const float pos = posSm[o];
                const float fpos = pos * (kFrames - 1); const int f0 = int(fpos), f1 = std::min(kFrames - 1, f0 + 1); const float ff = fpos - f0;
                for (int u = 0; u < uni; ++u)
                {
                    // unison: symmetric detune, centre voice undetuned; stereo spread by index
                    const float det = uni == 1 ? 0.0f : ((u / float(uni - 1)) * 2.0f - 1.0f) * osc.spread * 0.6f;
                    // drift: slow random wander
                    if ((age & 63) == 0) { driftState[o][u] += (driftTarget[o][u] - driftState[o][u]) * driftK * 64.0f; if (rng.nextFloat() < 0.002f) driftTarget[o][u] = rng.nextFloat() * 2.0f - 1.0f; }
                    const float semis = basePitch + det + driftState[o][u] * p.drift * 0.25f;
                    const float hz = 440.0f * std::pow(2.0f, (semis - 69.0f) / 12.0f);
                    const float inc = hz / float(fs);
                    // continuous mip: crossfade between the two octave bands around this pitch, so band-limiting and
                    // pitch-tracked formants both move smoothly (no steps under vibrato, glide or unison detune)
                    const float xm = std::log2(std::max(hz, 1.0f) / kMipBaseHz);           // band m covers (m-1, m]
                    const int mA = juce::jlimit(0, kMips - 1, int(std::ceil(xm))); const int mB = std::min(kMips - 1, mA + 1);
                    const float mf = juce::jlimit(0.0f, 1.0f, xm - float(mA - 1));
                    const float x = ph[o][u] * kFrameLen; const int xi = int(x); const float xf = x - xi;
                    auto rd = [&](int m) { const float* fa = tb->frame(m, f0); const float* fb = tb->frame(m, f1);
                                           const float va = fa[xi] + (fa[xi + 1] - fa[xi]) * xf, vb = fb[xi] + (fb[xi + 1] - fb[xi]) * xf; return va + (vb - va) * ff; };
                    const float v = (mA == mB ? rd(mA) : rd(mA) * (1.0f - mf) + rd(mB) * mf) * osc.level / std::sqrt(float(uni));
                    ph[o][u] += inc; if (ph[o][u] >= 1.0f) ph[o][u] -= 1.0f;
                    const float pan = zw::clampf(uni == 1 ? 0.0f : ((u / float(uni - 1)) * 2.0f - 1.0f) * osc.width, -1.0f, 1.0f);
                    sL += v * std::sqrt(0.5f * (1.0f - pan)); sR += v * std::sqrt(0.5f * (1.0f + pan));
                }
            }
            sL *= 0.7f; sR *= 0.7f;
            if (p.evolve > 0.0005f)   // slow stereo drift + gentle level swell per note
            {
                const float pan = ev[3] * 0.35f, lvl = 1.0f + 0.12f * ev[2];
                const float gl = std::sqrt(0.5f * (1.0f - pan)) * 1.4142f * lvl, gr = std::sqrt(0.5f * (1.0f + pan)) * 1.4142f * lvl;
                sL *= gl; sR *= gr;
            }
            // breath: noise through two slowly wandering vocal-formant bandpasses, keyed to the note's loudness
            if (p.breath > 0.0005f)
            {
                if ((age & 255) == 0)
                    for (int c = 0; c < 2; ++c) { const float w = 1.0f + 0.12f * driftState[1][c]; breathBp[c][0].setCutoff(breathF[0] * w); breathBp[c][1].setCutoff(breathF[1] / w); }
                const float nz = rng.nextFloat() * 2.0f - 1.0f;
                for (int c = 0; c < 2; ++c)
                {
                    const float nl = breathLp[c].process(nz);
                    const float b = (breathBp[c][0].process(nl) + 0.7f * breathBp[c][1].process(nl) + 0.08f * nl) * p.breath * 0.35f;
                    (c == 0 ? sL : sR) += b;
                }
            }
            // drive + filter
            if (p.drive > 0.001f) { const float g = 1.0f + p.drive * 6.0f; sL = zw::fastTanh(sL * g) / std::sqrt(g); sR = zw::fastTanh(sR * g) / std::sqrt(g); }
            sL = filt[0].process(sL); sR = filt[1].process(sR);
            const float vg = a * (0.3f + 0.7f * velocity);
            L[i] += sL * vg; R[i] += sR * vg;
            ++age;
        }
    }
    float ampLevel() const { return amp.value(); }
    float currentPitch() const { return pitch; }

private:
    double fs = 48000.0; const VoiceParams* params = nullptr; const std::shared_ptr<Table>* tables = nullptr;
    int note = -1, age = 0; float velocity = 1.0f, pitch = 60.0f, targetPitch = 60.0f, glideK = 1.0f; bool released = false;
    Adsr amp, mod; zw::SVF filt[2];
    float ph[2][8] {}, driftState[2][8] {}, driftTarget[2][8] {}; float driftK = 0.001f, posK = 0.003f, posSm[2] = { -1.0f, -1.0f };
    float vibAmt = 0.0f, vibPhase = 0.0f, vibRate = 5.2f, vibOnsetK = 0.0001f, breathF[2] = { 1000.0f, 2500.0f };
    float evRate[4] = { 0, 0, 0, 0 }, evPhase[4] = { 0, 0.25f, 0.5f, 0.75f }, ev[4] = { 0, 0, 0, 0 };
    zw::SVF breathBp[2][2]; zw::OnePole breathLp[2];
    juce::Random rng;
};


// ---------------- internal FX chain: EQ -> SATURATE -> GLUE -> ECHO (SPACE follows in the processor) --------
struct EqNode { bool on = false; int type = 0; float freq = 1000.0f, gainDb = 0.0f, q = 1.0f; };   // type: 0 bell 1 low shelf 2 high shelf 3 low cut 4 high cut

class Biquad
{
public:
    void reset() { for (auto& c : ch) c = {}; }
    void set(const EqNode& n, double fs)
    {
        const float A = std::pow(10.0f, n.gainDb / 40.0f), w0 = 2.0f * 3.14159265f * zw::clampf(n.freq, 10.0f, float(fs * 0.45)) / float(fs);
        const float cw = std::cos(w0), sw = std::sin(w0), alpha = sw / (2.0f * zw::clampf(n.q, 0.1f, 18.0f));
        float b0, b1, b2, a0, a1, a2;
        switch (n.type)
        {
            case 1: { const float s2 = 2.0f * std::sqrt(A) * alpha; b0 = A * ((A + 1) - (A - 1) * cw + s2); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - s2); a0 = (A + 1) + (A - 1) * cw + s2; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - s2; } break;
            case 2: { const float s2 = 2.0f * std::sqrt(A) * alpha; b0 = A * ((A + 1) + (A - 1) * cw + s2); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - s2); a0 = (A + 1) - (A - 1) * cw + s2; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - s2; } break;
            case 3: b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case 4: b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            default: b0 = 1 + alpha * A; b1 = -2 * cw; b2 = 1 - alpha * A; a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
        }
        c0 = b0 / a0; c1 = b1 / a0; c2 = b2 / a0; d1 = a1 / a0; d2 = a2 / a0;
    }
    float process(int c, float x) { auto& s = ch[c]; const float y = c0 * x + c1 * s.x1 + c2 * s.x2 - d1 * s.y1 - d2 * s.y2; s.x2 = s.x1; s.x1 = x; s.y2 = s.y1; s.y1 = y; return y; }
    // magnitude response in dB (for drawing)
    float magDb(float hz, double fs) const
    {
        const float w = 2.0f * 3.14159265f * hz / float(fs);
        const std::complex<float> z1(std::cos(-w), std::sin(-w)), z2 = z1 * z1;
        const auto num = c0 + c1 * z1 + c2 * z2, den = 1.0f + d1 * z1 + d2 * z2;
        return 20.0f * std::log10(std::abs(num / den) + 1e-9f);
    }
private:
    struct S { float x1 = 0, x2 = 0, y1 = 0, y2 = 0; } ch[2];
    float c0 = 1, c1 = 0, c2 = 0, d1 = 0, d2 = 0;
};

struct FxParams
{
    EqNode eq[4];
    bool satOn = false; int satType = 0; float satDrive = 0.3f, satTone = 0.0f, satBias = 0.0f, satLowHz = 20.0f, satOutDb = 0.0f, satMix = 1.0f;
    bool glueOn = false; float glueDepth = 0.5f, glueTime = 0.5f, glueUp = 1.0f, glueDown = 1.0f, glueMix = 1.0f, glueOutDb = 0.0f;
    bool chorusOn = false; int chorusMode = 2; float chorusRate = 0.5f, chorusDepth = 0.5f, chorusMix = 0.5f, chorusFb = 0.0f;   // mode: 0 Chorus I, 1 Chorus II, 2 Ensemble, 3 Flanger, 4 Phaser
    bool echoOn = false; float echoTimeMs = 375.0f; float echoFeedback = 0.4f, echoMix = 0.25f, echoTone = 0.5f, echoWobble = 0.0f, echoWidth = 0.66f; bool echoPingPong = true;
};

// waveshapers shared with CORE's band shaper: 0 Tape 1 Tube 2 Fold 3 Rectify 4 Crush(handled outside) 5 Diode 6 Octave 7 Hard
inline float fxShape(int type, float v) noexcept
{
    switch (type)
    {
        case 1:  { const float t = zw::fastTanh(v * 0.7f); return t + 0.18f * t * t * (v > 0 ? 1.0f : -1.0f); }
        case 2:  return std::sin(zw::clampf(v, -6.2f, 6.2f) * 0.9f) * 0.95f;
        case 3:  { const float r = v >= 0 ? v : -v * 0.35f; return zw::fastTanh(r * 1.2f) - 0.25f; }
        case 5:  { const float a = v >= 0 ? 1.0f : 2.2f; return zw::fastTanh(v * a) / (v >= 0 ? 1.0f : 1.6f); }
        case 6:  { const float r = std::fabs(v); return zw::fastTanh(r * 1.5f) * 0.9f - 0.45f; }
        case 7:  return zw::clampf(v, -0.8f, 0.8f) * 1.1f;
        default: return zw::fastTanh(v);
    }
}

class FxChain
{
public:
    void prepare(double sr)
    {
        fs = sr;
        for (auto& b : eq) b.reset();
        glue.prepare(sr, 6.0f, 90.0f);
        for (int c = 0; c < 2; ++c)
        {
            dl[c].prepare(sr, 2.5f); tone[c].setSampleRate(sr); tone[c].setLowpass(8000.0f); hp[c].setSampleRate(sr); hp[c].setHighpass(120.0f);
            satDC[c].setSampleRate(sr); satDC[c].setHighpass(10.0f); satLow[c].setSampleRate(sr); satLow[c].setLowpass(20.0f); satTilt[c].setSampleRate(sr); satTilt[c].reset(); satOS[c].reset();
        }
        wobble.setSampleRate(sr); wobble.setRate(0.23f); wobble.setShape(zw::LFO::Shape::Sine); wobble.reset(0.0f);
        for (int c = 0; c < 2; ++c) { chDl[c].prepare(sr, 0.06f); chTone[c].setSampleRate(sr); chTone[c].setLowpass(9000.0f); }
        for (int i = 0; i < 3; ++i) { chLfo[i].setSampleRate(sr); chLfo[i].setShape(zw::LFO::Shape::Sine); chLfo[i].reset(i / 3.0f); }
        smChMix.setSampleRate(sr); smChMix.setTime(40.0f); smChMix.reset(0.0f);
        for (int c = 0; c < 2; ++c) { flFb[c] = 0.0f; phFb[c] = 0.0f; for (auto& a : ph1[c]) a = 0.0f; for (auto& a : ph1y[c]) a = 0.0f; }
        for (auto* s : { &smSatMix, &smGlueMix, &smEchoMix, &smEchoFb, &smEchoTime, &smSatDrive, &smSatOut, &smGlueOut, &smCrush }) { s->setSampleRate(sr); s->setTime(30.0f); }
        smEchoTime.setTime(80.0f);
        set(params); smEchoTime.reset(float(params.echoTimeMs * 0.001 * sr)); smEchoMix.reset(0.0f); smSatMix.reset(0.0f); smGlueMix.reset(0.0f); smSatOut.reset(1.0f); smGlueOut.reset(1.0f);
    }
    void set(const FxParams& p)
    {
        params = p;
        for (int i = 0; i < 4; ++i) { eqOn[i] = p.eq[i].on; if (p.eq[i].on) eq[i].set(p.eq[i], fs); }
        smSatMix.setTarget(p.satOn ? p.satMix : 0.0f); smSatDrive.setTarget(p.satDrive); smSatOut.setTarget(zw::dbToGain(p.satOutDb));
        for (auto& f : satLow) f.setLowpass(zw::clampf(p.satLowHz, 20.0f, 500.0f));
        for (auto& t : satTilt) t.setTilt(p.satTone);
        smCrush.setTarget(p.satType == 4 ? 1.0f : 0.0f);
        smGlueMix.setTarget(p.glueOn ? p.glueMix : 0.0f); smGlueOut.setTarget(zw::dbToGain(p.glueOutDb)); glue.set(p.glueDepth, p.glueTime, 1.0f, 1.0f, true); glue.setBalance(p.glueUp, p.glueDown);
        smChMix.setTarget(p.chorusOn ? p.chorusMix : 0.0f);
        { const float r = p.chorusMode >= 3 ? 0.03f + p.chorusRate * p.chorusRate * 1.5f : 0.08f + p.chorusRate * p.chorusRate * 2.5f; chLfo[0].setRate(r); chLfo[1].setRate(r * 1.31f); chLfo[2].setRate(r * 0.77f); }
        smEchoMix.setTarget(p.echoOn ? p.echoMix : 0.0f); smEchoFb.setTarget(zw::clampf(p.echoFeedback, 0.0f, 0.95f));
        smEchoTime.setTarget(float(zw::clampf(p.echoTimeMs, 10.0f, 2400.0f) * 0.001 * fs));
        for (auto& t : tone) t.setLowpass(zw::lerp(1500.0f, 14000.0f, p.echoTone));
    }
    void process(float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            // EQ
            for (int k = 0; k < 4; ++k) if (eqOn[k]) { l = eq[k].process(0, l); r = eq[k].process(1, r); }
            // Saturate: lows below LOW stay clean, TONE tilts into the shaper, 2x oversampled waveshaper with BIAS, OUT trim, MIX
            {
                const float m = smSatMix.next(), d = smSatDrive.next(), og = smSatOut.next(), crush = smCrush.next();
                if (m > 0.0005f)
                {
                    const float g = 1.0f + d * d * 14.0f, bias = params.satBias * 0.6f, norm = 1.0f / std::sqrt(g);
                    const bool split = params.satLowHz > 22.0f;
                    float in[2] = { l, r }, outv[2];
                    for (int c = 0; c < 2; ++c)
                    {
                        const float low = split ? satLow[c].process(in[c]) : 0.0f;
                        float x = satTilt[c].process(in[c] - low);
                        float y;
                        if (crush > 0.5f)   // Crush: sample/bit reduction, no oversampling needed (the aliasing *is* the character)
                        {
                            const float steps = 4.0f + (1.0f - d) * 60.0f; const float q = std::round(x * g * steps) / steps;
                            if (++crushCnt[c] >= 1 + int(d * 6.0f)) { crushCnt[c] = 0; crushHold[c] = q; }
                            y = crushHold[c] * norm;
                        }
                        else
                        {
                            float u0, u1; satOS[c].upsample(x * g, u0, u1);
                            const int t = params.satType;
                            u0 = fxShape(t, u0 + bias) - fxShape(t, bias); u1 = fxShape(t, u1 + bias) - fxShape(t, bias);
                            y = satOS[c].downsample(u0, u1) * norm;
                        }
                        outv[c] = satDC[c].process(y) * og + low;
                    }
                    l = zw::lerp(l, outv[0], m); r = zw::lerp(r, outv[1], m);
                }
            }
            // Glue (single-band OTT, adaptive, UP / DOWN balance, parallel MIX, OUT)
            { const float m = smGlueMix.next(), og = smGlueOut.next(); const float g = glue.process(l, r); if (m > 0.0005f) { const float gg = zw::lerp(1.0f, g * og, m); l *= gg; r *= gg; } glueGrVis = glue.gainReductionDb(); }
            // Chorus / Ensemble (BBD-style modulated delays; I = one voice, II = deeper/slower, Ensemble = three voices)
            {
                const float m = smChMix.next();
                chDl[0].write(chTone[0].process(l + flFb[0])); chDl[1].write(chTone[1].process(r + flFb[1]));
                if (m > 0.0005f)
                {
                    const float l0 = chLfo[0].next(), l1 = chLfo[1].next(), l2 = chLfo[2].next();
                    const float base = (params.chorusMode == 1 ? 11.0f : 7.0f) * 0.001f * float(fs), depth = params.chorusDepth * (params.chorusMode == 1 ? 4.5f : 3.0f) * 0.001f * float(fs);
                    float wl, wr;
                    if (params.chorusMode == 3)          // Flanger: 0.6 .. 6 ms, feedback, R sweeps opposite
                    {
                        const float fb = zw::clampf(params.chorusFb, -0.9f, 0.9f);
                        const float dmin = 0.0006f * float(fs), dspan = 0.0055f * float(fs) * (0.2f + 0.8f * params.chorusDepth);
                        const float dl0 = dmin + dspan * (0.5f + 0.5f * l0), dr0 = dmin + dspan * (0.5f - 0.5f * l0);
                        wl = chDl[0].read(dl0); wr = chDl[1].read(dr0);
                        flFb[0] = wl * fb; flFb[1] = wr * fb;          // fed into the next write below
                    }
                    else if (params.chorusMode == 4)     // Phaser: 6 first-order allpass stages, centre swept 200 Hz .. 4 kHz
                    {
                        const float fb = zw::clampf(params.chorusFb, -0.9f, 0.9f);
                        float in[2] = { l + phFb[0], r + phFb[1] }, outv[2];
                        for (int c = 0; c < 2; ++c)
                        {
                            const float lf = c == 0 ? l0 : l1;
                            const float hz = 200.0f * std::pow(20.0f, 0.5f + 0.5f * lf * params.chorusDepth);
                            const float tn = std::tan(3.14159265f * zw::clampf(hz, 40.0f, float(fs * 0.45)) / float(fs)); const float a = (tn - 1.0f) / (tn + 1.0f);
                            float x = in[c];
                            for (int st = 0; st < 6; ++st) { const float y = a * x + ph1[c][st] - a * ph1y[c][st]; ph1[c][st] = x; ph1y[c][st] = y; x = y; }
                            outv[c] = x; phFb[c] = x * fb;
                        }
                        wl = outv[0]; wr = outv[1];
                    }
                    else if (params.chorusMode == 2)
                    {
                        wl = (chDl[0].read(base + depth * l0) + chDl[0].read(base * 1.6f + depth * l2) + chDl[1].read(base * 1.3f - depth * l1)) * 0.45f;
                        wr = (chDl[1].read(base - depth * l0) + chDl[1].read(base * 1.6f - depth * l2) + chDl[0].read(base * 1.3f + depth * l1)) * 0.45f;
                    }
                    else { wl = chDl[0].read(base + depth * l0); wr = chDl[1].read(base - depth * l0); }
                    l = l * (1.0f - 0.5f * m) + wl * m; r = r * (1.0f - 0.5f * m) + wr * m;
                }
                else { chLfo[0].next(); chLfo[1].next(); chLfo[2].next(); flFb[0] = flFb[1] = 0.0f; phFb[0] = phFb[1] = 0.0f; }
            }
            // Echo (ping-pong, filtered feedback)
            {
                const float m = smEchoMix.next(), fb = smEchoFb.next();
                const float wob = wobble.next() * params.echoWobble * 0.006f * float(fs);     // up to +-6 ms tape wow
                const float tSamp = std::max(2.0f, smEchoTime.next() + wob);
                const float dL = dl[0].read(tSamp), dR = dl[1].read(tSamp * (params.echoPingPong ? 1.0f : 1.0f));
                const float inL = hp[0].process(l), inR = hp[1].process(r);
                const float fbL = tone[0].process(params.echoPingPong ? dR : dL) * fb, fbR = tone[1].process(params.echoPingPong ? dL : dR) * fb;
                dl[0].write(zw::fastTanh(inL + fbL)); dl[1].write(zw::fastTanh(inR + fbR));
                const float mid = 0.5f * (dL + dR), side = 0.5f * (dL - dR) * params.echoWidth * 1.5f;
                l += (mid + side) * m; r += (mid - side) * m;
            }
            L[i] = l; R[i] = r;
        }
    }
    float eqResponseDb(float hz) const { float db = 0.0f; for (int k = 0; k < 4; ++k) if (eqOn[k]) db += eq[k].magDb(hz, fs); return db; }
    float glueGrVis = 0.0f;
private:
    double fs = 48000.0; FxParams params;
    Biquad eq[4]; bool eqOn[4] = { false, false, false, false };
    zw::OttBand glue;
    zw::DelayLine dl[2]; zw::OnePole tone[2], hp[2], satDC[2], satLow[2]; zw::TiltEQ satTilt[2]; zw::Oversampler2x satOS[2]; zw::LFO wobble;
    zw::DelayLine chDl[2]; zw::OnePole chTone[2]; zw::LFO chLfo[3]; zw::Smoother smChMix;
    float flFb[2] = { 0, 0 }, phFb[2] = { 0, 0 }, ph1[2][6] {}, ph1y[2][6] {};
    float crushHold[2] = { 0, 0 }; int crushCnt[2] = { 0, 0 };
    zw::Smoother smSatMix, smSatDrive, smSatOut, smCrush, smGlueMix, smGlueOut, smEchoMix, smEchoFb, smEchoTime;
};

} // namespace nebula
