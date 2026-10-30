// Minimal dependency-free test runner for the ZenWave DSP library.
// Checks the properties a main-stage plugin must have: no NaNs, bounded output,
// stable feedback, working freeze, and sensible silence behaviour.

#include <zenwave/zenwave.h>
#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } else { std::printf("ok:   %s\n", msg); } } while (0)

static bool finiteAndBounded(const std::vector<float>& v, float bound)
{
    for (float x : v) if (!std::isfinite(x) || std::fabs(x) > bound) return false;
    return true;
}

static float peak(const std::vector<float>& v)
{
    float p = 0.0f; for (float x : v) p = std::max(p, std::fabs(x)); return p;
}

static float rms(const std::vector<float>& v, size_t from)
{
    double s = 0; size_t n = 0;
    for (size_t i = from; i < v.size(); ++i) { s += double(v[i]) * v[i]; ++n; }
    return n ? float(std::sqrt(s / n)) : 0.0f;
}

int main()
{
    const double sr = 48000.0;
    const int block = 512;

    // --- Saturator: bounded, odd/even behaviour, no DC blow-up --------------------
    {
        zw::Saturator s; s.setSampleRate(sr); s.setDrive(1.0f); s.setBias(0.3f);
        std::vector<float> out;
        for (int i = 0; i < 48000; ++i) out.push_back(s.process(std::sin(2 * zw::kPi * 55.0f * i / float(sr)) * 2.0f));
        CHECK(finiteAndBounded(out, 3.0f), "Saturator output is finite and bounded at max drive");
        float mean = 0; for (float x : out) mean += x; mean /= out.size();
        CHECK(std::fabs(mean) < 0.05f, "Saturator DC blocker removes bias offset");
    }

    // --- SpaceReverb: impulse decays, freeze sustains ------------------------------
    {
        zw::SpaceReverb r; r.prepare(sr); r.setDecay(0.6f); r.setSize(0.6f);
        std::vector<float> L(int(sr * 4)), R(L.size());
        for (size_t i = 0; i < L.size(); ++i) { float a = 0, b = 0; r.process(i == 0 ? 1.0f : 0.0f, i == 0 ? 1.0f : 0.0f, a, b); L[i] = a; R[i] = b; }
        CHECK(finiteAndBounded(L, 2.0f) && finiteAndBounded(R, 2.0f), "Reverb impulse response is finite and bounded");
        CHECK(rms(L, L.size() - 4800) < rms(L, 4800) , "Reverb tail decays over 4 s");
        CHECK(peak(L) > 0.01f, "Reverb produces a tail");

        zw::SpaceReverb f; f.prepare(sr); f.setDecay(0.8f);
        std::vector<float> F(int(sr * 6));
        for (size_t i = 0; i < F.size(); ++i)
        {
            if (i == size_t(sr)) f.setFreeze(true);
            float a = 0, b = 0;
            const float in = (i < 2000) ? std::sin(2 * zw::kPi * 220.0f * i / float(sr)) : 0.0f;
            f.process(in, in, a, b); F[i] = a;
        }
        const float early = rms(F, size_t(sr * 1.5)), late = rms(F, size_t(sr * 5.5));
        CHECK(late > early * 0.5f, "Freeze holds the tail (late RMS stays within 6 dB of early)");
        CHECK(finiteAndBounded(F, 2.0f), "Freeze stays bounded");
    }

    // --- DriftEngine: silence in → silence out, feedback > 1 stays bounded ---------
    {
        zw::DriftEngine d; d.prepare(sr, block);
        zw::DriftParams p; p.feedback = 1.1f; p.age = 1.0f; p.wowFlutter = 1.0f; p.space = 1.0f; p.mix = 1.0f;
        d.setParams(p);
        std::vector<float> L(size_t(sr * 8), 0.0f), R(L.size(), 0.0f);
        // one-second burst then silence
        for (size_t i = 0; i < size_t(sr); ++i) { L[i] = R[i] = 0.8f * std::sin(2 * zw::kPi * 40.0f * i / float(sr)); }
        for (size_t i = 0; i < L.size(); i += block)
            d.process(L.data() + i, R.data() + i, int(std::min<size_t>(block, L.size() - i)));
        CHECK(finiteAndBounded(L, 4.0f) && finiteAndBounded(R, 4.0f), "Drift at 110% feedback, full age, full space stays bounded");
        CHECK(peak(L) > 0.05f, "Drift produces repeats");

        zw::DriftEngine q; q.prepare(sr, block);
        zw::DriftParams z; z.mix = 0.5f; q.setParams(z);
        std::vector<float> ZL(size_t(sr * 2), 0.0f), ZR(ZL.size(), 0.0f);
        for (size_t i = 0; i < ZL.size(); i += block)
            q.process(ZL.data() + i, ZR.data() + i, int(std::min<size_t>(block, ZL.size() - i)));
        CHECK(peak(ZL) < 1.0e-3f, "Drift is near-silent with silent input (hiss floor only)");
    }


    // --- UmbraEngine: bounded at extremes, sub band passes through untouched -------
    {
        zw::UmbraEngine u; u.prepare(sr, block);
        zw::UmbraParams p; p.decay = 1.0f; p.size = 1.0f; p.dark = 1.0f; p.density = 1.0f; p.motion = 1.0f; p.mix = 1.0f; p.width = 1.5f;
        u.setParams(p);
        std::vector<float> L(size_t(sr * 6), 0.0f), R(L.size(), 0.0f);
        for (size_t i = 0; i < size_t(sr); ++i) L[i] = R[i] = 0.9f * std::sin(2 * zw::kPi * 1000.0f * i / float(sr));
        for (size_t i = 0; i < L.size(); i += block) u.process(L.data() + i, R.data() + i, int(std::min<size_t>(block, L.size() - i)));
        CHECK(finiteAndBounded(L, 4.0f) && finiteAndBounded(R, 4.0f), "Umbra at max decay/size/dark/density/motion stays bounded");
        CHECK(rms(L, size_t(sr * 1.2)) > 1.0e-4f, "Umbra produces a tail");

        // 35 Hz sine with gravity crossover at ~170 Hz and mix 100%: output should still be ~the dry sub
        zw::UmbraEngine g; g.prepare(sr, block);
        zw::UmbraParams q; q.mix = 1.0f; q.gravity = 0.5f; g.setParams(q);
        std::vector<float> SL(size_t(sr * 2)), SR(SL.size());
        for (size_t i = 0; i < SL.size(); ++i) SL[i] = SR[i] = 0.5f * std::sin(2 * zw::kPi * 35.0f * i / float(sr));
        for (size_t i = 0; i < SL.size(); i += block) g.process(SL.data() + i, SR.data() + i, int(std::min<size_t>(block, SL.size() - i)));
        const float subRms = rms(SL, size_t(sr));
        CHECK(subRms > 0.30f && subRms < 0.40f, "Umbra Gravity keeps a 35 Hz sub essentially dry (RMS ~0.35)");
    }


    // --- CutFilter: slope accuracy one octave past cutoff --------------------------
    {
        auto measure = [&](zw::CutFilter::Slope sl, float hz) {
            zw::CutFilter f; f.setSampleRate(sr); f.setType(zw::CutFilter::Type::Highcut); f.setSlope(sl); f.setCutoff(1000.0f);
            double acc = 0; int n = 0;
            for (int i = 0; i < 48000; ++i) { float y = f.process(std::sin(2 * zw::kPi * hz * i / float(sr))); if (i > 8000) { acc += y * y; ++n; } }
            return 10.0 * std::log10(acc / n / 0.5);
        };
        const double a6 = measure(zw::CutFilter::Slope::dB6, 4000.0f), a12 = measure(zw::CutFilter::Slope::dB12, 4000.0f),
                     a24 = measure(zw::CutFilter::Slope::dB24, 4000.0f), a48 = measure(zw::CutFilter::Slope::dB48, 4000.0f);
        std::printf("      highcut @1k, measured 2 octaves up: 6:%.1f 12:%.1f 24:%.1f 48:%.1f dB\n", a6, a12, a24, a48);
        CHECK(std::fabs(a6 + 12.3) < 2.5,  "CutFilter 6 dB/oct ≈ -12 dB two octaves up");
        CHECK(std::fabs(a12 + 24.1) < 3.0, "CutFilter 12 dB/oct ≈ -24 dB two octaves up");
        CHECK(std::fabs(a24 + 48.2) < 4.0, "CutFilter 24 dB/oct ≈ -48 dB two octaves up");
        CHECK(a48 < -80.0,                 "CutFilter 48 dB/oct < -80 dB two octaves up");
        const double pass = measure(zw::CutFilter::Slope::dB48, 200.0f);
        CHECK(std::fabs(pass) < 0.5, "CutFilter passband flat (48 dB/oct, 200 Hz within ±0.5 dB)");
    }

    // --- Oversampled saturator: alias rejection ------------------------------------
    {
        auto aliasAt = [&](int os) {
            zw::Saturator sat; sat.setSampleRate(sr); sat.setDrive(0.5f); sat.setCharacter(zw::Saturator::Character::Tape); sat.setEmphasis(0.0f); sat.setOversampling(os);
            const int N = 1 << 15; std::vector<float> y(N);
            for (int i = 0; i < N; ++i) y[i] = sat.process(std::sin(2 * zw::kPi * 9000.0f * i / float(sr)));
            // 5th harmonic = 45 kHz → aliases to 3 kHz at 48 kHz
            double re = 0, im = 0; for (int i = N / 2; i < N; ++i) { re += y[i] * std::cos(2 * zw::kPi * 3000.0f * i / float(sr)); im += y[i] * std::sin(2 * zw::kPi * 3000.0f * i / float(sr)); }
            return 20.0 * std::log10(std::sqrt(re * re + im * im) / (N / 2) + 1e-12);
        };
        const double direct = aliasAt(1), over2 = aliasAt(2), over4 = aliasAt(4);
        std::printf("      3 kHz alias product: direct %.1f dB, 2x %.1f dB, 4x %.1f dB\n", direct, over2, over4);
        CHECK(over4 < direct - 15.0, "4x oversampled saturator cuts alias products by > 15 dB");
    }

    // --- True-stereo reverb: left-only input stays left-weighted early ------------
    {
        zw::SpaceReverb r; r.prepare(sr); r.setDecay(0.5f);
        double eL = 0, eR = 0;
        for (int i = 0; i < int(sr * 0.25); ++i)
        {
            float a = 0, b = 0;
            const float in = (i < 400) ? std::sin(2 * zw::kPi * 500.0f * i / float(sr)) : 0.0f;
            r.process(in, 0.0f, a, b);
            if (i > 400 && i < int(sr * 0.08)) { eL += a * a; eR += b * b; }
        }
        std::printf("      early energy L/R ratio: %.1f dB\n", 10 * std::log10(eL / (eR + 1e-12)));
        CHECK(eL > eR * 1.5, "True-stereo reverb keeps a left-only source left-weighted in the early tail");
    }

    // --- DelayLine: fractional read is accurate for a ramp -------------------------
    {
        zw::DelayLine dl; dl.prepare(sr, 0.1f);
        for (int i = 0; i < 1000; ++i) dl.write(float(i));
        const float r = dl.read(10.5f);
        CHECK(std::fabs(r - (999.0f - 9.5f)) < 0.01f, "DelayLine Hermite read interpolates a ramp exactly");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
