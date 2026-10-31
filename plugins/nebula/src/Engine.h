#pragma once
// NEBULA engine: planetary wavetables + harmonic carving + per-voice filter/drive + space.
//
// Tables are generated procedurally from harmonic recipes (no sample data), rendered by IFFT into
// 32 frames x 6 band-limited mip levels. CARVE is a 32-band per-harmonic gain curve applied to the
// recipe before rendering, so carving changes the wave itself. Re-rendering a table takes ~2 ms and
// happens on the message thread; the audio thread swaps in the new table atomically.

#include <JuceHeader.h>
#include <zenwave/zenwave.h>
#include <array>
#include <atomic>
#include <memory>
#include <cmath>

namespace nebula {

constexpr int kFrameLen = 1024, kFrames = 32, kMips = 6, kPartials = 384, kCarveBands = 8, kTables = 8;

inline const juce::StringArray& tableNames() { static const juce::StringArray n { "Glass", "Voice", "Metal", "Grain", "Dune", "Halo", "Ion", "Void" }; return n; }

// Harmonic recipe: amplitude + phase per partial for a given table and frame position (0..1)
inline void recipe(int table, float pos, float* amp, float* phase, juce::Random& rng)
{
    for (int h = 0; h < kPartials; ++h) { amp[h] = 0.0f; phase[h] = 0.0f; }
    auto set = [&](int h, float a, float ph = 0.0f) { if (h >= 1 && h <= kPartials) { amp[h - 1] = a; phase[h - 1] = ph; } };
    const float t = pos;
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
                const float f = h * 110.0f;   // nominal at A2
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
            juce::Random rng(1234 + table * 99 + f);   // deterministic per frame
            recipe(table, float(f) / (kFrames - 1), amp.data(), phase.data(), rng);
            if (carveOn) for (int h = 0; h < kPartials; ++h) amp[size_t(h)] *= carveGainAt(carve, h + 1);
            for (int m = 0; m < kMips; ++m)
            {
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
                float pk = 1e-6f; for (int i = 0; i < kFrameLen; ++i) pk = std::max(pk, std::fabs(time[size_t(i)].real()));
                for (int i = 0; i < kFrameLen; ++i) dst[i] = time[size_t(i)].real() / pk * 0.9f;
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
        if (silent) { for (auto& f : filt) f.reset(); posSm[0] = posSm[1] = -1.0f; }
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
                float hz = p.filterHz * std::pow(2.0f, kt + p.envAmt * 6.0f * m + pressure * 2.0f);
                hz = zw::clampf(hz, 20.0f, float(fs * 0.45));
                for (auto& f : filt) { f.setCutoff(hz); f.setResonance(0.5f + p.res * 9.5f); f.setMode(p.filterMode == 0 ? zw::SVF::Mode::Lowpass : p.filterMode == 1 ? zw::SVF::Mode::Highpass : zw::SVF::Mode::Bandpass); }
            }
            float sL = 0.0f, sR = 0.0f;
            for (int o = 0; o < 2; ++o)
            {
                const auto& osc = p.osc[o];
                if (! osc.on || osc.level <= 0.0005f) continue;
                const Table* tb = tables[o].get(); if (! tb) continue;
                const float basePitch = pitch + osc.octave * 12.0f + osc.semis + osc.fine * 0.01f + bendSemis;
                const int uni = juce::jlimit(1, 8, osc.unison);
                const float posTarget = zw::clampf(osc.position + p.lfoToPosition * 0.5f * lfoPos, 0.0f, 1.0f);
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
                    // mip by frequency: partials allowed = Nyquist / hz
                    const float maxPartials = float(fs) * 0.5f / std::max(1.0f, hz);
                    int mip = 0; while (mip < kMips - 1 && (kFrameLen / 2 >> mip) > maxPartials) ++mip;
                    const float* fa = tb->frame(mip, f0); const float* fb = tb->frame(mip, f1);
                    const float x = ph[o][u] * kFrameLen; const int xi = int(x); const float xf = x - xi;
                    const float va = fa[xi] + (fa[xi + 1] - fa[xi]) * xf, vb = fb[xi] + (fb[xi + 1] - fb[xi]) * xf;
                    const float v = (va + (vb - va) * ff) * osc.level / std::sqrt(float(uni));
                    ph[o][u] += inc; if (ph[o][u] >= 1.0f) ph[o][u] -= 1.0f;
                    const float pan = zw::clampf(uni == 1 ? 0.0f : ((u / float(uni - 1)) * 2.0f - 1.0f) * osc.width, -1.0f, 1.0f);
                    sL += v * std::sqrt(0.5f * (1.0f - pan)); sR += v * std::sqrt(0.5f * (1.0f + pan));
                }
            }
            sL *= 0.7f; sR *= 0.7f;
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
    juce::Random rng;
};

} // namespace nebula
