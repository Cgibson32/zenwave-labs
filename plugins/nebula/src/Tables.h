// ZenWave Labs - NEBULA wavetable library (tables 8 and up)
// Everything here is procedural: vocal tables are built from measured vowel formants (five formants per vowel,
// four voice types), atmospheric tables from slowly evolving spectral envelopes, and the rest from classic
// additive recipes. No sample data ships with the plugin, so the whole library costs nothing on disk and the
// CARVE bands can be folded straight into the harmonic recipe before rendering.
#pragma once
#include <cmath>
#include <algorithm>
#include <array>
#include <juce_core/juce_core.h>

namespace nebula
{
constexpr int kLegacyTables = 8;

// ---- names (index = table parameter value; the first 8 are the original set and must keep their order) ----
inline const juce::StringArray& allTableNames()
{
    static const juce::StringArray n {
        // 0..7 original
        "Glass", "Voice", "Metal", "Grain", "Dune", "Halo", "Ion", "Void",
        // 8..23 vocal
        "Ahh Choir", "Ooh Choir", "Mmm Hum", "Om Chant", "Soul Vox", "Alto Breath", "Soprano Air", "Bass Monk",
        "Whisper", "Vowel Sweep", "Gospel Oh", "Lullaby", "Throat Drone", "Siren Vox", "Choir Stack", "Angelic",
        // 24..30 atmosphere
        "Aurora", "Nimbus", "Stratus", "Solaris", "Tide", "Deep Field", "Dust",
        // 31..35 orchestral
        "Ensemble", "Cello Body", "Brass Swell", "Flute", "Bowed Glass",
        // 36..40 bells / keys
        "Celesta", "Music Box", "Kalimba", "Crystal", "Tine",
        // 41..45 analogue / digital
        "Super Saw", "PWM", "Tri Fold", "Drawbars", "FM Bell",
        // 46..48 sub / bass
        "Deep Sub", "Reese", "808 Tone" };
    return n;
}
// section headings for the table menus: { first index, title }
inline const std::vector<std::pair<int, const char*>>& tableSections()
{
    static const std::vector<std::pair<int, const char*>> s { { 0, "CLASSIC" }, { 8, "VOCAL" }, { 24, "ATMOSPHERE" }, { 31, "ORCHESTRAL" }, { 36, "BELLS + KEYS" }, { 41, "ANALOGUE" }, { 46, "SUB" } };
    return s;
}

// ---- vowel formant data: F1..F5 Hz, amplitude dB, bandwidth Hz (classic soprano/alto/tenor/bass sets) ----
struct Formant { float f[5], db[5], bw[5]; };
enum VoiceType { Soprano = 0, Alto, Tenor, Bass };
enum Vowel { A = 0, E, I, O, U };
inline const Formant& formant(int voice, int vowel)
{
    static const Formant tbl[4][5] = {
        { // soprano
          { { 800, 1150, 2900, 3900, 4950 }, { 0, -6, -32, -20, -50 }, { 80, 90, 120, 130, 140 } },
          { { 350, 2000, 2800, 3600, 4950 }, { 0, -20, -15, -40, -56 }, { 60, 100, 120, 150, 200 } },
          { { 270, 2140, 2950, 3900, 4950 }, { 0, -12, -26, -26, -44 }, { 60, 90, 100, 120, 120 } },
          { { 450, 800, 2830, 3800, 4950 }, { 0, -11, -22, -22, -50 }, { 70, 80, 100, 130, 135 } },
          { { 325, 700, 2700, 3800, 4950 }, { 0, -16, -35, -40, -60 }, { 50, 60, 170, 180, 200 } } },
        { // alto
          { { 800, 1150, 2800, 3500, 4950 }, { 0, -4, -20, -36, -60 }, { 80, 90, 120, 130, 140 } },
          { { 400, 1600, 2700, 3300, 4950 }, { 0, -24, -30, -35, -60 }, { 60, 80, 120, 150, 200 } },
          { { 350, 1700, 2700, 3700, 4950 }, { 0, -20, -30, -36, -60 }, { 50, 100, 120, 150, 200 } },
          { { 450, 800, 2830, 3500, 4950 }, { 0, -9, -16, -28, -55 }, { 70, 80, 100, 130, 135 } },
          { { 325, 700, 2530, 3500, 4950 }, { 0, -12, -30, -40, -64 }, { 50, 60, 170, 180, 200 } } },
        { // tenor
          { { 650, 1080, 2650, 2900, 3250 }, { 0, -6, -7, -8, -22 }, { 80, 90, 120, 130, 140 } },
          { { 400, 1700, 2600, 3200, 3580 }, { 0, -14, -12, -14, -20 }, { 70, 80, 100, 120, 120 } },
          { { 290, 1870, 2800, 3250, 3540 }, { 0, -15, -18, -20, -30 }, { 40, 90, 100, 120, 120 } },
          { { 400, 800, 2600, 2800, 3000 }, { 0, -10, -12, -12, -26 }, { 70, 80, 100, 130, 135 } },
          { { 350, 600, 2700, 2900, 3300 }, { 0, -20, -17, -14, -26 }, { 40, 60, 100, 120, 120 } } },
        { // bass
          { { 600, 1040, 2250, 2450, 2750 }, { 0, -7, -9, -9, -20 }, { 60, 70, 110, 120, 130 } },
          { { 400, 1620, 2400, 2800, 3100 }, { 0, -12, -9, -12, -18 }, { 40, 80, 100, 120, 120 } },
          { { 250, 1750, 2600, 3050, 3340 }, { 0, -30, -16, -22, -28 }, { 60, 90, 100, 120, 120 } },
          { { 400, 750, 2400, 2600, 2900 }, { 0, -11, -21, -20, -40 }, { 40, 80, 100, 120, 120 } },
          { { 350, 600, 2400, 2675, 2950 }, { 0, -20, -32, -28, -36 }, { 40, 80, 100, 120, 120 } } } };
    return tbl[juce::jlimit(0, 3, voice)][juce::jlimit(0, 4, vowel)];
}
// interpolate between two vowels of one voice (formants move continuously - that is what a mouth does)
inline Formant blendFormant(int voice, int v0, int v1, float t)
{
    const Formant& a = formant(voice, v0); const Formant& b = formant(voice, v1); Formant r;
    for (int i = 0; i < 5; ++i) { r.f[i] = a.f[i] + (b.f[i] - a.f[i]) * t; r.db[i] = a.db[i] + (b.db[i] - a.db[i]) * t; r.bw[i] = a.bw[i] + (b.bw[i] - a.bw[i]) * t; }
    return r;
}
// walk a vowel sequence with position 0..1
inline Formant vowelPath(int voice, std::initializer_list<int> seq, float pos)
{
    const int n = int(seq.size()); if (n == 1) return formant(voice, *seq.begin());
    const float x = juce::jlimit(0.0f, 0.9999f, pos) * float(n - 1); const int i = int(x); const float t = x - float(i);
    return blendFormant(voice, *(seq.begin() + i), *(seq.begin() + i + 1), t);
}
// frame-independent pseudo-random phase: keeps morphs free of cancellation while flattening the crest factor
inline float fixedPhase(int h, int seed) { unsigned x = unsigned(h * 2654435761u) ^ unsigned(seed * 40503u); x ^= x >> 13; x *= 0x5bd1e995u; x ^= x >> 15; return float(x & 0xffff) / 65536.0f * 6.2831853f; }

// vocal source x formant filter -> partial amplitudes at nominal fundamental f0
// air: breathy high-partial noise 0..1, tilt: extra -dB/oct, hum: closed-mouth lowpass 0..1
inline void vocalSpectrum(const Formant& F, float f0, float* amp, float* phase, int seed, float air = 0.0f, float hum = 0.0f, float glottal = 1.0f, int nPartials = 160)
{
    for (int h = 1; h <= nPartials; ++h)
    {
        const float f = h * f0;
        float src = 1.0f / std::pow(float(h), 0.85f + 0.4f * glottal);          // glottal pulse ~ -12 dB/oct
        float res = 0.0f;
        for (int i = 0; i < 5; ++i)
        {
            const float bw = F.bw[i] * 1.3f, d = (f - F.f[i]) / bw;
            res += std::pow(10.0f, F.db[i] / 20.0f) / (1.0f + d * d);       // Lorentzian resonance
        }
        float a = src * (0.015f + res);
        a *= 1.0f / (1.0f + std::pow(f / 6500.0f, 4.0f));                     // vocal tract rolloff
        if (hum > 0.0f) a *= 1.0f / (1.0f + hum * std::pow(f / 900.0f, 2.0f)); // closed mouth
        if (air > 0.0f && f > 1800.0f) { const float r = 0.5f + 0.5f * std::sin(float(h) * 12.9898f + seed); a += air * 0.012f * r / std::pow(f / 1800.0f, 0.7f); }
        amp[h - 1] = a; phase[h - 1] = fixedPhase(h, seed);
    }
    // body: a sung note always carries its fundamental - keep it within 4 dB of the loudest formant partial
    float mx = 0.0f; for (int h = 0; h < nPartials; ++h) mx = std::max(mx, amp[h]);
    amp[0] = std::max(amp[0], mx * 0.63f);
}

inline void recipeEx(int table, float t, float* amp, float* phase, juce::Random& rng)
{
    auto set = [&](int h, float a, float ph = 0.0f) { if (h >= 1 && h <= 384) { amp[h - 1] = a; phase[h - 1] = ph; } };
    auto bell = [](float x, float c, float w) { const float d = (x - c) / w; return std::exp(-d * d); };
    const float pi = 3.14159265f;
    switch (table)
    {
        // ------------------------------------------------------------------ VOCAL
        case 8:  vocalSpectrum(vowelPath(Alto, { A, A, O }, t), 220.0f, amp, phase, 8, 0.15f * t); break;                   // Ahh Choir: open ah drifting to oh
        case 9:  vocalSpectrum(vowelPath(Alto, { O, U }, t), 220.0f, amp, phase, 9, 0.1f); break;                            // Ooh Choir
        case 10: vocalSpectrum(vowelPath(Tenor, { U, U, O }, t), 110.0f, amp, phase, 10, 0.0f, 1.2f * (1.0f - t)); break;    // Mmm Hum: closed hum opening
        case 11: vocalSpectrum(vowelPath(Bass, { O, U, U }, t), 110.0f, amp, phase, 11, 0.0f, 0.9f * t, 1.2f); break;        // Om Chant: oh -> mm
        case 12: vocalSpectrum(vowelPath(Tenor, { E, A, O }, t), 165.0f, amp, phase, 12, 0.08f, 0.0f, 0.8f); break;          // Soul Vox
        case 13: vocalSpectrum(vowelPath(Alto, { A, O }, t), 220.0f, amp, phase, 13, 0.25f + 0.6f * t); break;               // Alto Breath
        case 14: vocalSpectrum(vowelPath(Soprano, { I, E }, t), 330.0f, amp, phase, 14, 0.45f, 0.0f, 0.6f); break;           // Soprano Air
        case 15: vocalSpectrum(vowelPath(Bass, { U, O, A }, t), 82.0f, amp, phase, 15, 0.0f, 0.0f, 1.3f); break;             // Bass Monk
        case 16: vocalSpectrum(vowelPath(Alto, { A, E, I, O, U }, t), 220.0f, amp, phase, 16, 1.0f, 0.0f, 0.3f); break;      // Whisper: mostly air
        case 17: vocalSpectrum(vowelPath(Tenor, { A, E, I, O, U }, t), 147.0f, amp, phase, 17, 0.05f); break;                // Vowel Sweep
        case 18: vocalSpectrum(vowelPath(Tenor, { A, O, O }, t), 196.0f, amp, phase, 18, 0.1f, 0.0f, 0.9f); break;           // Gospel Oh
        case 19: vocalSpectrum(vowelPath(Soprano, { U, U, O }, t), 262.0f, amp, phase, 19, 0.05f, 0.5f, 1.4f); break;        // Lullaby: soft, few harmonics
        case 20: // Throat Drone: bass 'a' whose emphasised overtone climbs 4 -> 12 (overtone singing)
        {
            vocalSpectrum(formant(Bass, A), 82.0f, amp, phase, 20, 0.0f, 0.0f, 1.1f);
            const float hEmph = 4.0f + 8.0f * t;
            for (int h = 1; h <= 160; ++h) amp[h - 1] *= 1.0f + 6.0f * bell(float(h), hEmph, 0.6f);
            break;
        }
        case 21: // Siren Vox: soprano a -> i with a blooming 5th/octave cluster
        {
            vocalSpectrum(vowelPath(Soprano, { A, I }, t), 330.0f, amp, phase, 21, 0.2f, 0.0f, 0.7f);
            for (int h : { 3, 4, 6, 8 }) amp[h - 1] += 0.08f * t;
            break;
        }
        case 22: // Choir Stack: four voice types singing the same vowel path, summed
        {
            std::array<float, 384> a2{}, p2{}; std::fill(amp, amp + 384, 0.0f);
            const int voices[4] = { Soprano, Alto, Tenor, Bass }; const float f0s[4] = { 330.0f, 220.0f, 165.0f, 110.0f };
            for (int v = 0; v < 4; ++v)
            {
                vocalSpectrum(vowelPath(voices[v], { A, O, U }, t), f0s[v], a2.data(), p2.data(), 22 + v, 0.1f);
                for (int h = 0; h < 384; ++h) amp[h] += a2[size_t(h)] * (v == 3 ? 0.7f : 1.0f);
            }
            for (int h = 1; h <= 384; ++h) phase[h - 1] = fixedPhase(h, 22);
            break;
        }
        case 23: // Angelic: soprano ee/eh with glass partials on 2, 3, 4, 6, 8
        {
            vocalSpectrum(vowelPath(Soprano, { E, I }, t), 330.0f, amp, phase, 23, 0.3f, 0.0f, 0.6f);
            for (int h : { 2, 3, 4, 6, 8, 12 }) amp[h - 1] += 0.06f * (0.4f + 0.6f * t) / std::sqrt(float(h));
            break;
        }
        // ------------------------------------------------------------------ ATMOSPHERE
        case 24: // Aurora: three soft spectral bands that drift apart with position, odd-dominant
            for (int h = 1; h <= 96; ++h)
            {
                const float lh = std::log2(float(h));
                const float a = (h == 1 ? 1.0f : 0.0f) + 0.6f * bell(lh, 1.0f + 0.6f * t, 0.5f) + 0.4f * bell(lh, 2.6f + 1.2f * t, 0.6f) + 0.25f * bell(lh, 4.2f + 1.0f * t, 0.7f);
                set(h, a / std::pow(float(h), 0.5f) * (h % 2 ? 1.0f : 0.6f), fixedPhase(h, 24));
            }
            break;
        case 25: // Nimbus: a cloud - fundamental plus a dense, soft, fixed-phase haze whose brightness rises
            for (int h = 1; h <= 200; ++h)
            {
                const float haze = 0.12f * std::exp(-float(h) * (0.06f - 0.045f * t)) * (0.6f + 0.4f * std::sin(h * 0.37f));
                set(h, (h == 1 ? 1.0f : 0.0f) + (h == 2 ? 0.3f : 0.0f) + haze, fixedPhase(h, 25));
            }
            break;
        case 26: // Stratus: string-like saw with two moving spectral notches
            for (int h = 1; h <= 128; ++h)
            {
                const float lh = std::log2(float(h));
                const float notch = (1.0f - 0.9f * bell(lh, 2.0f + 2.0f * t, 0.25f)) * (1.0f - 0.8f * bell(lh, 4.5f - 1.5f * t, 0.3f));
                set(h, notch / std::pow(float(h), 1.05f), fixedPhase(h, 26));
            }
            break;
        case 27: // Solaris: organ-ish sine with sparse upper partials arriving one by one
        {
            const int order[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };
            for (int i = 0; i < 8; ++i) { const float on = juce::jlimit(0.0f, 1.0f, t * 9.0f - i); set(order[i], (i == 0 ? 1.0f : 0.55f / std::sqrt(float(i + 1))) * on, 0.3f * i); }
            break;
        }
        case 28: // Tide: breathy two-formant sweep (not a vowel - wider, airier)
            for (int h = 1; h <= 160; ++h)
            {
                const float f = h * 110.0f;
                const float form = bell(f, 400.0f + 900.0f * t, 260.0f) + 0.5f * bell(f, 3200.0f - 1200.0f * t, 700.0f);
                set(h, (0.03f + form) / std::pow(float(h), 0.55f), fixedPhase(h, 28));
            }
            break;
        case 29: // Deep Field: sub-sine; 3rd and 5th unfold slowly, then a faint high shimmer
            for (int h = 1; h <= 40; ++h)
            {
                float a = h == 1 ? 1.0f : 0.0f;
                if (h == 2) a = 0.25f; if (h == 3) a = 0.3f * juce::jlimit(0.0f, 1.0f, t * 2.0f); if (h == 5) a = 0.18f * juce::jlimit(0.0f, 1.0f, t * 2.0f - 0.4f);
                if (h >= 8) a = 0.04f * juce::jlimit(0.0f, 1.0f, t * 2.0f - 1.0f) * std::exp(-0.08f * (h - 8));
                set(h, a, fixedPhase(h, 29));
            }
            break;
        case 30: // Dust: grainy comb that slides, fixed phases so it stays a texture, not a buzz
            for (int h = 1; h <= 180; ++h)
            {
                const float comb = 0.5f + 0.5f * std::cos(float(h) * (0.6f + 2.4f * t));
                set(h, (h == 1 ? 1.0f : 0.0f) + 0.25f * comb * std::exp(-0.012f * h), fixedPhase(h, 30));
            }
            break;
        // ------------------------------------------------------------------ ORCHESTRAL
        case 31: // Ensemble: saw through a string-body hump (300 / 1000 Hz) that opens with position
            for (int h = 1; h <= 160; ++h)
            {
                const float f = h * 220.0f;
                const float body = 0.6f + 0.8f * bell(f, 300.0f, 200.0f) + 0.6f * bell(f, 1000.0f, 500.0f) + 0.4f * t * bell(f, 2800.0f, 900.0f);
                set(h, body / std::pow(float(h), 1.0f - 0.25f * t), fixedPhase(h, 31) * 0.3f);
            }
            break;
        case 32: // Cello Body: saw, strong low formants, gentle highs
            for (int h = 1; h <= 120; ++h)
            {
                const float f = h * 110.0f;
                const float body = 0.5f + 1.2f * bell(f, 220.0f, 120.0f) + 0.8f * bell(f, 500.0f, 250.0f) + 0.4f * bell(f, 1400.0f, 600.0f);
                set(h, body / std::pow(float(h), 1.1f - 0.2f * t), fixedPhase(h, 32) * 0.2f);
            }
            break;
        case 33: // Brass Swell: saw whose upper spectrum blooms with position (the brass crescendo)
            for (int h = 1; h <= 160; ++h) set(h, 1.0f / std::pow(float(h), 1.6f - 1.0f * t) * (1.0f + 0.4f * t * bell(std::log2(float(h)), 3.0f, 0.8f)));
            break;
        case 34: // Flute: near-sine with 2nd/3rd and breath that grows
            for (int h = 1; h <= 60; ++h)
            {
                float a = h == 1 ? 1.0f : h == 2 ? 0.35f : h == 3 ? 0.15f : h == 4 ? 0.06f : 0.0f;
                if (h > 4) a = 0.02f * t * (0.5f + 0.5f * std::sin(h * 1.7f)) * std::exp(-0.05f * h);
                set(h, a, fixedPhase(h, 34));
            }
            break;
        case 35: // Bowed Glass: odd harmonics, stretched spacing creeping in with position
            for (int k = 1; k <= 30; ++k)
            {
                const int h = int(std::round((2 * k - 1) * (1.0f + 0.004f * t * k * k)));
                set(h, std::exp(-0.12f * k) * (k == 1 ? 1.0f : 0.7f), fixedPhase(k, 35) * 0.5f);
            }
            break;
        // ------------------------------------------------------------------ BELLS + KEYS
        case 36: // Celesta: fundamental + bell partials at 4, 10, 12 (inharmonic feel) brightening
            set(1, 1.0f); set(4, 0.35f + 0.3f * t); set(10, 0.15f + 0.25f * t, 1.0f); set(12, 0.08f + 0.15f * t, 2.0f); set(17, 0.05f * t, 0.5f); set(24, 0.04f * t); break;
        case 37: // Music Box: bright plucky partials on 1, 3, 6, 9, 14
            set(1, 1.0f); set(3, 0.5f); set(6, 0.25f + 0.2f * t); set(9, 0.12f + 0.15f * t); set(14, 0.06f + 0.1f * t); set(20, 0.05f * t); break;
        case 38: // Kalimba: hollow fundamental, odd tine partials
            set(1, 1.0f); set(2, 0.08f); set(3, 0.1f + 0.2f * t); set(5, 0.06f + 0.15f * t); set(7, 0.04f + 0.1f * t); set(11, 0.05f * t); break;
        case 39: // Crystal: pure high clusters (sine -> glittering overtone stack)
            for (int h = 1; h <= 48; ++h) { const float g = (h == 1 ? 1.0f : 0.0f) + t * 0.3f * bell(std::log2(float(h)), 3.0f + 1.5f * t, 0.7f) * (h % 2 ? 1.0f : 0.7f); set(h, g, fixedPhase(h, 39)); }
            break;
        case 40: // Tine: electric-piano tine - sine with a tine formant at ~8x that adds bark
            for (int h = 1; h <= 32; ++h) { float a = h == 1 ? 1.0f : h == 2 ? 0.12f : 0.0f; a += (0.08f + 0.35f * t) * bell(float(h), 8.0f, 1.5f) + 0.05f * t * bell(float(h), 14.0f, 2.0f); set(h, a, fixedPhase(h, 40)); }
            break;
        // ------------------------------------------------------------------ ANALOGUE
        case 41: // Super Saw: saw plus faux-detuned sidebands (per-partial amplitude ripple that shifts with position)
            for (int h = 1; h <= 200; ++h) { const float ripple = 0.75f + 0.25f * std::cos(float(h) * (1.3f + 2.0f * t)); set(h, ripple / h, fixedPhase(h, 41) * 0.6f); }
            break;
        case 42: // PWM: pulse width 50% -> 5%
            for (int h = 1; h <= 200; ++h) { const float pw = 0.5f - 0.45f * t; set(h, std::fabs(std::sin(pi * h * pw)) / h); }
            break;
        case 43: // Tri Fold: triangle that wavefolds (adds odd harmonics in a folded pattern)
            for (int h = 1; h <= 120; h += 2) { const float tri = 1.0f / float(h * h); const float fold = (0.5f + 0.5f * std::cos(h * 0.35f * (1.0f + 4.0f * t))) / h; set(h, tri + t * fold, (h / 2) % 2 ? pi : 0.0f); }
            break;
        case 44: // Drawbars: 16' 8' 5 1/3' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1' sliding from a soft flute registration to full
        {
            const int hs[9] = { 1, 2, 3, 4, 6, 8, 10, 12, 16 }; const float soft[9] = { 0.6f, 1.0f, 0.0f, 0.35f, 0.0f, 0.1f, 0, 0, 0 }, full[9] = { 0.9f, 1.0f, 0.7f, 0.8f, 0.6f, 0.6f, 0.4f, 0.4f, 0.5f };
            for (int i = 0; i < 9; ++i) set(hs[i], soft[i] + (full[i] - soft[i]) * t, 0.2f * i);
            break;
        }
        case 45: // FM Bell: 2-op FM, ratio 3.5, index rising with position (Bessel sidebands approximated by a sinc-ish spread)
        {
            const float idx = 0.3f + 6.0f * t, ratio = 3.5f;
            for (int h = 1; h <= 160; ++h)
            {
                float a = 0.0f;
                for (int k = -8; k <= 8; ++k) { const float fk = 1.0f + k * ratio; if (std::fabs(fk - h) < 0.5f) { const float env = std::exp(-std::pow(float(std::abs(k)) / (idx + 0.3f), 2.0f)); a += env; } }
                set(h, a, fixedPhase(h, 45));
            }
            break;
        }
        // ------------------------------------------------------------------ SUB
        case 46: // Deep Sub: sine that gains a touch of 2nd/3rd (tape-ish warmth) with position
            set(1, 1.0f); set(2, 0.12f * t); set(3, 0.08f * t); set(5, 0.02f * t); break;
        case 47: // Reese: two detuned saws can't live in one cycle, so: saw with phase-alternating harmonic ripple that darkens
            for (int h = 1; h <= 120; ++h) { const float ripple = 0.55f + 0.45f * std::cos(float(h) * (0.9f + 1.4f * t) + 0.7f); set(h, ripple / std::pow(float(h), 1.0f + 0.5f * t), (h % 2) ? 0.0f : pi * 0.5f); }
            break;
        default: // 808 Tone: sine with soft 2nd and a hint of 3rd, slight saturation with position
            set(1, 1.0f); set(2, 0.2f + 0.2f * t); set(3, 0.05f + 0.15f * t); set(4, 0.04f * t); break;
    }
    juce::ignoreUnused(rng);
}
} // namespace nebula
