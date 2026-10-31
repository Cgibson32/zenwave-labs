#pragma once
// ZenWave modulation matrix: two tempo-syncable LFOs + an envelope follower, routed to
// any continuous parameter through N slots. Works in the normalised parameter domain so
// it is plugin-agnostic. Processor side: call process() per sub-block, then value(t).
// Editor side: visual atomics (lfo/env values, per-target modulated position).

#include <juce_audio_processors/juce_audio_processors.h>
#include "ProcessorHelpers.h"
#include <array>
#include <cmath>
#include <vector>

namespace zwui {

struct ModIDs
{
    static constexpr int kSlots = 4;
    static juce::String lfoShape(int i) { return "lfo" + juce::String(i + 1) + "shape"; }
    static juce::String lfoSync (int i) { return "lfo" + juce::String(i + 1) + "sync"; }
    static juce::String lfoDiv  (int i) { return "lfo" + juce::String(i + 1) + "div"; }
    static juce::String lfoRate (int i) { return "lfo" + juce::String(i + 1) + "rate"; }
    static constexpr auto envAttack  = "envattack";
    static constexpr auto envDecay   = "envdecay";
    static constexpr auto envSustain = "envsustain";
    static constexpr auto envRelease = "envrelease";
    static constexpr auto envSource  = "envsource";   // 0 = this plugin's input, 1 = sidechain input
    static juce::String slotSrc(int i) { return "mod" + juce::String(i + 1) + "src"; }
    static juce::String slotDst(int i) { return "mod" + juce::String(i + 1) + "dst"; }
    static juce::String slotAmt(int i) { return "mod" + juce::String(i + 1) + "amt"; }

    static juce::StringArray shapeNames() { return { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random", "Drift", "Steps" }; }
    static constexpr int kSteps = 16;
    static juce::String step(int lfo, int i) { return "lfo" + juce::String(lfo + 1) + "step" + juce::String(i + 1); }
    static juce::String glide(int lfo) { return "lfo" + juce::String(lfo + 1) + "glide"; }
    static juce::StringArray sourceNames() { return { "Off", "LFO1", "LFO2", "ENV" }; }
};

// A modulation target: a continuous parameter the matrix may move.
struct ModTarget { const char* id; const char* name; };

inline void addModParams(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const std::vector<ModTarget>& targets)
{
    using P = juce::AudioParameterFloat; using B = juce::AudioParameterBool; using C = juce::AudioParameterChoice; using Rng = juce::NormalisableRange<float>;
    juce::StringArray dstNames { "Off" }; for (auto& t : targets) dstNames.add(t.name);
    const auto divNames = divisionNames();
    for (int i = 0; i < 2; ++i)
    {
        const auto n = "LFO " + juce::String(i + 1) + " ";
        params.push_back(std::make_unique<C>(juce::ParameterID{ ModIDs::lfoShape(i), 1 }, n + "Shape", ModIDs::shapeNames(), 0));
        params.push_back(std::make_unique<B>(juce::ParameterID{ ModIDs::lfoSync(i), 1 }, n + "Sync", true));
        params.push_back(std::make_unique<C>(juce::ParameterID{ ModIDs::lfoDiv(i), 1 }, n + "Division", divNames, i == 0 ? 8 : 10));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::lfoRate(i), 1 }, n + "Rate", Rng{ 0.02f, 20.0f, 0.001f, 0.35f }, i == 0 ? 0.5f : 0.12f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 2) + " Hz"; })));
    }
    auto ms = [](float v, int) { return v < 100.0f ? juce::String(v, 1) + " ms" : juce::String(juce::roundToInt(v)) + " ms"; };
    params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::envAttack, 1 }, "Env Attack", Rng{ 0.5f, 500.0f, 0.1f, 0.4f }, 8.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction(ms)));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::envRelease, 1 }, "Env Release", Rng{ 10.0f, 2000.0f, 0.1f, 0.4f }, 180.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction(ms)));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ModIDs::envSource, 1 }, "Env Source", juce::StringArray{ "Input", "Sidechain" }, 0));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::envDecay, 1 }, "Env Decay", Rng{ 5.0f, 2000.0f, 0.1f, 0.4f }, 120.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction(ms)));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::envSustain, 1 }, "Env Sustain", Rng{ 0.0f, 1.0f, 0.001f }, 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v * 100)) + " %"; })));
    // Each LFO has its own 16-step lane for the "Steps" shape (one pass = the LFO's division)
    for (int l = 0; l < 2; ++l)
    {
        for (int i = 0; i < ModIDs::kSteps; ++i)
            params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::step(l, i), 1 }, "LFO " + juce::String(l + 1) + " Step " + juce::String(i + 1), Rng{ -1.0f, 1.0f, 0.001f },
                l == 0 ? ((i % 4 == 0) ? 1.0f : (i % 2 == 0 ? 0.0f : -0.5f)) : (1.0f - 2.0f * float(i) / 15.0f),
                juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v * 100)) + " %"; })));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::glide(l), 1 }, "LFO " + juce::String(l + 1) + " Glide", Rng{ 0.0f, 1.0f, 0.001f }, 0.1f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v * 100)) + " %"; })));
    }
    for (int i = 0; i < ModIDs::kSlots; ++i)
    {
        const auto n = "Mod " + juce::String(i + 1) + " ";
        params.push_back(std::make_unique<C>(juce::ParameterID{ ModIDs::slotSrc(i), 1 }, n + "Source", ModIDs::sourceNames(), 0));
        params.push_back(std::make_unique<C>(juce::ParameterID{ ModIDs::slotDst(i), 1 }, n + "Target", dstNames, 0));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ModIDs::slotAmt(i), 1 }, n + "Amount", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v * 100)) + " %"; })));
    }
}

// Reset every modulation parameter to its default (factory preset recall)
inline void resetModParams(juce::AudioProcessorValueTreeState& apvts)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            if (rp->paramID.startsWith("lfo") || rp->paramID.startsWith("env") || rp->paramID.startsWith("mod"))
            { rp->beginChangeGesture(); rp->setValueNotifyingHost(rp->getDefaultValue()); rp->endChangeGesture(); }
}
// Set one slot of the matrix (used by factory presets that ship with motion)
inline void setModSlot(juce::AudioProcessorValueTreeState& apvts, int slot, int src, int dstIndex1Based, float amount)
{
    auto set = [&](const juce::String& id, float plain) { if (auto* rp = apvts.getParameter(id)) { rp->beginChangeGesture(); rp->setValueNotifyingHost(rp->convertTo0to1(plain)); rp->endChangeGesture(); } };
    set(ModIDs::slotSrc(slot), float(src)); set(ModIDs::slotDst(slot), float(dstIndex1Based)); set(ModIDs::slotAmt(slot), amount);
}
inline void setLfo(juce::AudioProcessorValueTreeState& apvts, int lfo, int shape, bool sync, int div, float rateHz)
{
    auto set = [&](const juce::String& id, float plain) { if (auto* rp = apvts.getParameter(id)) { rp->beginChangeGesture(); rp->setValueNotifyingHost(rp->convertTo0to1(plain)); rp->endChangeGesture(); } };
    set(ModIDs::lfoShape(lfo), float(shape)); set(ModIDs::lfoSync(lfo), sync ? 1.0f : 0.0f); set(ModIDs::lfoDiv(lfo), float(div)); set(ModIDs::lfoRate(lfo), rateHz);
}

inline void setEnv(juce::AudioProcessorValueTreeState& apvts, float atkMs, float decMs, float sus, float relMs)
{
    auto set = [&](const juce::String& id, float plain) { if (auto* rp = apvts.getParameter(id)) { rp->beginChangeGesture(); rp->setValueNotifyingHost(rp->convertTo0to1(plain)); rp->endChangeGesture(); } };
    set(ModIDs::envAttack, atkMs); set(ModIDs::envDecay, decMs); set(ModIDs::envSustain, sus); set(ModIDs::envRelease, relMs);
}
inline void setParamPlain(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float plain)
{ if (auto* rp = apvts.getParameter(id)) { rp->beginChangeGesture(); rp->setValueNotifyingHost(rp->convertTo0to1(plain)); rp->endChangeGesture(); } }

class ModMatrix
{
public:
    static constexpr int kMaxTargets = 32;

    void attach(juce::AudioProcessorValueTreeState& apvts, const std::vector<ModTarget>& targets)
    {
        numTargets = juce::jmin(kMaxTargets, int(targets.size()));
        for (int t = 0; t < numTargets; ++t) { target[size_t(t)] = apvts.getParameter(targets[size_t(t)].id); modPos[size_t(t)].store(-1.0f); }
        for (int i = 0; i < 2; ++i)
        {
            lfo[i].shape = apvts.getRawParameterValue(ModIDs::lfoShape(i));
            lfo[i].sync  = apvts.getRawParameterValue(ModIDs::lfoSync(i));
            lfo[i].div   = apvts.getRawParameterValue(ModIDs::lfoDiv(i));
            lfo[i].rate  = apvts.getRawParameterValue(ModIDs::lfoRate(i));
        }
        for (int l = 0; l < 2; ++l)
        {
            for (int i = 0; i < ModIDs::kSteps; ++i) lfo[l].stepP[i] = apvts.getRawParameterValue(ModIDs::step(l, i));
            lfo[l].glideP = apvts.getRawParameterValue(ModIDs::glide(l));
        }
        envAttack = apvts.getRawParameterValue(ModIDs::envAttack);
        envDecay = apvts.getRawParameterValue(ModIDs::envDecay);
        envSource = apvts.getRawParameterValue(ModIDs::envSource);
        envSustain = apvts.getRawParameterValue(ModIDs::envSustain);
        envRelease = apvts.getRawParameterValue(ModIDs::envRelease);
        for (int i = 0; i < ModIDs::kSlots; ++i)
        {
            slot[i].src = apvts.getRawParameterValue(ModIDs::slotSrc(i));
            slot[i].dst = apvts.getRawParameterValue(ModIDs::slotDst(i));
            slot[i].amt = apvts.getRawParameterValue(ModIDs::slotAmt(i));
        }
    }

    void prepare(double sampleRate) { sr = sampleRate; env = 0.0f; adsr = 0.0f; gate = false; stage = 0; slowPeak = 0.05f; for (auto& l : lfo) { l.phase = 0.0; l.rnd = 0.0f; l.rndTarget = 0.0f; l.rndPrev = 0.0f; } rng.setSeed(1234); }

    // Advance sources over one sub-block of input. ppq/playing come from the host transport.
    // scL/scR: the plugin's sidechain input for this sub-block (null when none is connected).
    void process(const float* inL, const float* inR, int n, double bpm, double ppq, bool playing,
                 const float* scL = nullptr, const float* scR = nullptr)
    {
        scPresent.store(scL != nullptr);
        if (envSource->load() > 0.5f && scL != nullptr) { inL = scL; inR = scR ? scR : scL; }
        // Input-triggered ADSR: a fast peak follower, normalised by a slow running peak (so it self-adjusts to
        // any input level), opens a gate; the envelope then runs Attack -> Decay -> Sustain while the gate is
        // open and Release when it closes. Retriggers on new hits.
        {
            const float fa = std::exp(-1.0f / (float(sr) * 0.001f)), fr = std::exp(-1.0f / (float(sr) * 0.040f));
            const float pkDecay = std::exp(-1.0f / (float(sr) * 1.5f));
            const float atk = std::exp(-1.0f / (float(sr) * envAttack->load() * 0.001f));
            const float dec = std::exp(-1.0f / (float(sr) * envDecay->load() * 0.001f));
            const float rel = std::exp(-1.0f / (float(sr) * envRelease->load() * 0.001f));
            const float sus = envSustain->load();
            for (int i = 0; i < n; ++i)
            {
                const float x = std::max(std::abs(inL[i]), std::abs(inR ? inR[i] : inL[i]));
                env = x > env ? fa * env + (1.0f - fa) * x : fr * env + (1.0f - fr) * x;
                slowPeak = std::max(env, slowPeak * pkDecay); if (slowPeak < 0.003f) slowPeak = 0.003f;   // ~-50 dBFS floor: silence never triggers
                const float lvl = env / slowPeak;                      // 0..1 relative to the recent loudest moment
                if (! gate && lvl > 0.45f && env > 0.004f) { gate = true; stage = 1; }
                else if (gate && lvl < 0.18f) { gate = false; stage = 3; }
                switch (stage)
                {
                    case 1: adsr = atk * adsr + (1.0f - atk) * 1.08f; if (adsr >= 1.0f) { adsr = 1.0f; stage = 2; } break;   // attack (slight overshoot target so it reaches 1)
                    case 2: adsr = dec * adsr + (1.0f - dec) * sus; break;                                                   // decay -> sustain
                    case 3: adsr = rel * adsr; if (adsr < 1e-4f) { adsr = 0.0f; stage = 0; } break;                         // release
                    default: break;
                }
            }
            src[3] = juce::jlimit(0.0f, 1.0f, adsr);
            gateVis.store(gate);
        }
        // LFOs
        for (int i = 0; i < 2; ++i)
        {
            auto& l = lfo[i];
            const bool sync = l.sync->load() > 0.5f;
            double cycleSec = sync ? divisionSeconds(int(l.div->load()), bpm) : 1.0 / double(l.rate->load());
            const double inc = double(n) / (sr * cycleSec);
            if (sync && playing)
            {
                const double beats = divisions()[size_t(juce::jlimit(0, int(divisions().size()) - 1, int(l.div->load())))].beats;
                l.phase = std::fmod(ppq / beats, 1.0); if (l.phase < 0) l.phase += 1.0;
            }
            else { l.phase += inc; }
            if (l.phase >= 1.0) { l.phase -= std::floor(l.phase); l.rndPrev = l.rndTarget; l.rndTarget = rng.nextFloat() * 2.0f - 1.0f; l.cycleCount++; }
            const float ph = float(l.phase);
            float v = 0.0f;
            switch (int(l.shape->load()))
            {
                case 0: v = std::sin(ph * juce::MathConstants<float>::twoPi); break;
                case 1: v = 1.0f - 4.0f * std::abs(ph - 0.5f); break;
                case 2: v = 2.0f * ph - 1.0f; break;
                case 3: v = 1.0f - 2.0f * ph; break;
                case 4: v = ph < 0.5f ? 1.0f : -1.0f; break;
                case 5: v = l.rndTarget; break;                                                      // sample & hold
                case 7: { const int st = juce::jlimit(0, ModIDs::kSteps - 1, int(ph * ModIDs::kSteps)); v = l.stepP[st]->load(); l.stepVis.store(st); } break;   // this LFO's step lane
                default: { const float t = ph * ph * (3.0f - 2.0f * ph); v = l.rndPrev + (l.rndTarget - l.rndPrev) * t; } break;  // smooth drift
            }
            // de-click square / S&H with a one-pole (≈ 3 ms); steps glide by a fraction of a step
            const int shp = int(l.shape->load());
            float k = std::exp(-float(n) / (float(sr) * 0.003f));
            if (shp == 7) { const float g = l.glideP->load(); const double stepSec = cycleSec / ModIDs::kSteps; k = std::exp(-float(n) / float(sr * std::max(0.002, stepSec * (0.03 + g * 0.9)))); }
            l.smooth = (shp >= 2 && shp != 6) ? k * l.smooth + (1.0f - k) * v : v;   // saws, square, S&H, steps: de-clicked
            src[1 + i] = l.smooth;
            l.phaseVis.store(ph);
        }
        for (int t = 0; t < 3; ++t) vis[size_t(t)].store(src[1 + t]);

        // sum contributions per target
        for (int t = 0; t < numTargets; ++t) offset[size_t(t)] = 0.0f;
        for (int s = 0; s < ModIDs::kSlots; ++s)
        {
            const int si = int(slot[s].src->load()), di = int(slot[s].dst->load()) - 1;
            if (si <= 0 || di < 0 || di >= numTargets) continue;
            offset[size_t(di)] += slot[s].amt->load() * src[si];
        }
        for (int t = 0; t < numTargets; ++t)
        {
            if (target[size_t(t)] == nullptr) continue;
            const float base = target[size_t(t)]->getValue();
            const float m = juce::jlimit(0.0f, 1.0f, base + offset[size_t(t)]);
            plain[size_t(t)] = target[size_t(t)]->convertFrom0to1(m);
            modPos[size_t(t)].store(hasMod(t) ? m : -1.0f);
        }
    }

    bool hasMod(int t) const
    {
        for (int s = 0; s < ModIDs::kSlots; ++s)
            if (int(slot[s].src->load()) > 0 && int(slot[s].dst->load()) - 1 == t && std::abs(slot[s].amt->load()) > 0.001f) return true;
        return false;
    }
    // Modulation span (normalised) for the editor's range arc: {lo, hi} around the base value
    std::pair<float, float> span(int t) const
    {
        float neg = 0.0f, pos = 0.0f;
        for (int s = 0; s < ModIDs::kSlots; ++s)
        {
            const int si = int(slot[s].src->load());
            if (si <= 0 || int(slot[s].dst->load()) - 1 != t) continue;
            const float a = slot[s].amt->load();
            if (si == 3) { if (a > 0) pos += a; else neg += a; }      // env: unipolar
            else { pos += std::abs(a); neg -= std::abs(a); }           // lfo: bipolar
        }
        const float base = target[size_t(t)] ? target[size_t(t)]->getValue() : 0.0f;
        return { juce::jlimit(0.0f, 1.0f, base + neg), juce::jlimit(0.0f, 1.0f, base + pos) };
    }

    // Modulated plain value of target t (valid after process())
    float value(int t) const { return plain[size_t(t)]; }
    // Raw normalised offset applied to target t (for params that are modulated differently when synced)
    float offsetNorm(int t) const { return juce::jlimit(-1.0f, 1.0f, offset[size_t(t)]); }
    // Plain value with no modulation, for params that aren't targets
    static float raw(const std::atomic<float>* p) { return p->load(); }

    std::array<std::atomic<float>, 3> vis {};                  // lfo1, lfo2, env (editor)
    std::array<std::atomic<float>, kMaxTargets> modPos {};     // normalised modulated pos per target, -1 = none
    float lfoPhase(int i) const { return lfo[i].phaseVis.load(); }
    int   lfoStep(int i) const { return lfo[i].stepVis.load(); }

private:
    struct Lfo { std::atomic<float>* shape{}, * sync{}, * div{}, * rate{}; std::atomic<float>* stepP[ModIDs::kSteps] {}; std::atomic<float>* glideP{}; double phase = 0; float rnd = 0, rndTarget = 0, rndPrev = 0, smooth = 0; int cycleCount = 0; std::atomic<float> phaseVis { 0.0f }; std::atomic<int> stepVis { 0 }; };
    struct Slot { std::atomic<float>* src{}, * dst{}, * amt{}; };
    Lfo lfo[2]; Slot slot[ModIDs::kSlots];
    std::atomic<float>* envAttack{}, * envRelease{}, * envDecay{}, * envSustain{}, * envSource{};
    float adsr = 0.0f, slowPeak = 0.05f; bool gate = false; int stage = 0;
public:
    std::atomic<bool> gateVis { false }, scPresent { false };
    bool usingSidechain() const { return envSource->load() > 0.5f; }
private:
    std::array<juce::RangedAudioParameter*, kMaxTargets> target {};
    std::array<float, kMaxTargets> offset {}, plain {};
    float src[4] = { 0, 0, 0, 0 };
    float env = 0.0f; double sr = 48000.0; int numTargets = 0;
    juce::Random rng;
};

} // namespace zwui
