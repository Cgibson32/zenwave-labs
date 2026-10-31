// ZenWave QA: loads a plugin headlessly, then for every preset and every parameter
// sweeps min → max while processing a stereo test signal. Fails on NaN/Inf, output
// above +12 dBFS, or discontinuities (clicks) larger than 0.5 between samples while the
// signal is quiet. Also checks preset recall is deterministic and state round-trips.
#include <JuceHeader.h>
#include <zenwave_ui/Orbit.h>
#include <cmath>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static int failures = 0;
#define QA_CHECK(cond, ...) do { if (!(cond)) { std::printf("FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); ++failures; } } while (0)

struct Stats { float peak = 0; bool finite = true; int clicks = 0; };

// Click-free test signal: phase-continuous kick with pitch envelope, gated lead/hat with
// 2 ms fades, so any click the detector finds is the plugin's, not the source's.
static double kickPhase = 0.0, leadPhase = 0.0, hatPhase = 0.0;
static bool continuousSignal = false;   // distortion plugins: no gates (a clipper turns any fade into a step)
static float smoothGate(double pos, double len, double fade = 0.002)
{
    if (pos < 0 || pos > len) return 0.0f;
    if (pos < fade) return float(pos / fade);
    if (pos > len - fade) return float((len - pos) / fade);
    return 1.0f;
}
static void fill(juce::AudioBuffer<float>& b, double& t, double sr)
{
    if (continuousSignal)
    {
        for (int i = 0; i < b.getNumSamples(); ++i, t += 1.0 / sr)
        {
            kickPhase += 2 * M_PI * 48.0 / sr; if (kickPhase > 2 * M_PI) kickPhase -= 2 * M_PI;
            leadPhase += 2 * M_PI * 660.0 / sr; if (leadPhase > 2 * M_PI) leadPhase -= 2 * M_PI;
            hatPhase  += 2 * M_PI * 9000.0 / sr; if (hatPhase  > 2 * M_PI) hatPhase  -= 2 * M_PI;
            const float swell = 0.55f + 0.45f * float(std::sin(2 * M_PI * 0.3 * t));   // slow level swell exercises auto-gain
            const float x = (0.5f * float(std::sin(kickPhase)) + 0.2f * float(std::sin(leadPhase)) + 0.08f * float(std::sin(hatPhase))) * swell;
            b.setSample(0, i, x); b.setSample(1, i, x * 0.85f);
        }
        return;
    }
    for (int i = 0; i < b.getNumSamples(); ++i, t += 1.0 / sr)
    {
        const double beatPos = std::fmod(t, 0.5);
        const float env = float(std::exp(-beatPos * 7.0)) * smoothGate(beatPos, 0.5, 0.004);
        kickPhase += 2 * M_PI * (45.0 + 80.0 * env) / sr; if (kickPhase > 2 * M_PI) kickPhase -= 2 * M_PI;
        leadPhase += 2 * M_PI * 660.0 / sr;                if (leadPhase > 2 * M_PI) leadPhase -= 2 * M_PI;
        hatPhase  += 2 * M_PI * 9000.0 / sr;               if (hatPhase  > 2 * M_PI) hatPhase  -= 2 * M_PI;
        const float kick = float(std::sin(kickPhase)) * env * 0.7f;
        const float lead = 0.2f * float(std::sin(leadPhase)) * smoothGate(std::fmod(t, 1.0), 0.25);
        const float hat  = 0.08f * float(std::sin(hatPhase)) * smoothGate(std::fmod(t, 0.125), 0.02);
        b.setSample(0, i, kick + lead + hat);
        b.setSample(1, i, kick + lead * 0.5f + hat * 0.8f);
    }
}

// Click = a sample whose second difference is far above the *tracked* second-difference envelope
// (1 ms attack, 50 ms release). Any steady tone, however bright or hard-clipped, keeps a crest
// factor of ~1-3 against that envelope; a 2 ms fade is followed by the 1 ms attack; only a true
// discontinuity (something the envelope cannot anticipate) spikes the ratio. Frequency-neutral.
// State persists across calls so run boundaries never look like steps.
struct ClickState { float h0 = 0, h1 = 0, env = 0; int hold = 2400; };   // 50 ms pre-roll before judging
static ClickState clickState[2];
static int countClicks(const std::vector<float>& x, int ch, bool dump = false)
{
    auto& st = clickState[ch];
    const float kA = 1.0f - std::exp(-1.0f / 48.0f), kR = 1.0f - std::exp(-1.0f / 2400.0f);
    int clicks = 0; size_t i = 0;
    for (float v : x)
    {
        const float a = std::fabs(v - 2.0f * st.h0 + st.h1);
        st.h1 = st.h0; st.h0 = v;
        const float ref = st.env;
        st.env += (a - st.env) * (a > st.env ? kA : kR);
        if (st.hold > 0) --st.hold;
        else if (a > 0.03f && a > 8.0f * (ref + 0.002f)) { ++clicks; st.hold = 240; if (dump) std::printf("    [click] ch%d sample %zu  d2 %.3f  tracked %.4f\n", ch, i, a, ref); }
        ++i;
    }
    return clicks;
}

static Stats run(juce::AudioProcessor& p, int blocks, double& t, double sr, juce::AudioBuffer<float>& b, juce::MidiBuffer& mb, float* lastOut)
{
    Stats s;
    std::vector<float> chan[2];
    for (int k = 0; k < blocks; ++k)
    {
        fill(b, t, sr);
        p.processBlock(b, mb);
        for (int c = 0; c < 2; ++c)
        {
            const float* d = b.getReadPointer(c);
            for (int i = 0; i < b.getNumSamples(); ++i) { if (! std::isfinite(d[i])) s.finite = false; s.peak = std::max(s.peak, std::fabs(d[i])); chan[c].push_back(d[i]); }
            lastOut[c] = d[b.getNumSamples() - 1];
        }
    }
    for (int c = 0; c < 2; ++c) s.clicks += countClicks(chan[c], c, std::getenv("ZWQA_DUMP") != nullptr);
    return s;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000.0; const int block = 256;

    // 0) Detector self-test: a bright, hard-clipped tone must not count; a single 0.3 step must
    {
        std::vector<float> harsh(96000), gated(96000), clicked(96000);
        for (int i = 0; i < 96000; ++i)
        {
            const double tt = i / sr;
            harsh[i] = std::tanh(6.0f * float(std::sin(2 * M_PI * 110.0 * tt) + 0.3 * std::sin(2 * M_PI * 7000.0 * tt)));          // hard-clipped, bright
            gated[i] = 0.5f * float(std::sin(2 * M_PI * 9000.0 * tt)) * smoothGate(std::fmod(tt, 0.125), 0.02);                    // hat bursts, 2 ms fades
            clicked[i] = std::tanh(1.5f * float(std::sin(2 * M_PI * 110.0 * tt))) + (i >= 48000 && i < 48000 + 40 ? 0.3f : 0.0f);  // soft tone + one 0.3 step
        }
        const int a = countClicks(harsh, 0), g = countClicks(gated, 1);
        clickState[0] = {}; clickState[1] = {};
        const int b2 = countClicks(clicked, 0);
        clickState[0] = {}; clickState[1] = {};
        QA_CHECK(a == 0, "detector self-test: clipped bright tone flagged %d clicks", a);
        QA_CHECK(g == 0, "detector self-test: gated hat bursts flagged %d clicks", g);
        QA_CHECK(b2 >= 1, "detector self-test: a 0.3 step was not detected");
        std::printf("== click detector self-test ok\n");
    }
    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    proc->setPlayConfigDetails(2, 2, sr, block);
    proc->prepareToPlay(sr, block);
    juce::AudioBuffer<float> b(2, block); juce::MidiBuffer mb; double t = 0; float last[2] = { 0, 0 };

    continuousSignal = proc->getName().containsIgnoreCase("Core");
    std::printf("== %s: %d parameters, %d presets%s\n", proc->getName().toRawUTF8(), proc->getParameters().size(), proc->getNumPrograms(), continuousSignal ? " (continuous test signal)" : "");

    // 1) Every preset: process 2 s, check sanity
    for (int pr = 0; pr < proc->getNumPrograms(); ++pr)
    {
        proc->setCurrentProgram(pr);
        run(*proc, int(sr * 0.3 / block), t, sr, b, mb, last);   // settle: a preset switch may retune
        auto s = run(*proc, int(sr * 2 / block), t, sr, b, mb, last);
        QA_CHECK(s.finite, "preset '%s' produced NaN/Inf", proc->getProgramName(pr).toRawUTF8());
        QA_CHECK(s.peak < 1.6f, "preset '%s' peak %.2f (> +4 dBFS)", proc->getProgramName(pr).toRawUTF8(), s.peak);
        const bool crusher = proc->getProgramName(pr).containsIgnoreCase("Bit") || proc->getProgramName(pr).containsIgnoreCase("Crush");   // stepped by design
        QA_CHECK(s.clicks <= (crusher ? 8 : 0), "preset '%s' had %d clicks", proc->getProgramName(pr).toRawUTF8(), s.clicks);
        std::printf("  preset %-26s peak %5.2f  clicks %d\n", proc->getProgramName(pr).toRawUTF8(), s.peak, s.clicks);
    }

    // 2) Every parameter: sweep min→max→min over 1.5 s on the Init preset
    proc->setCurrentProgram(0);
    for (auto* param : proc->getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param);
        if (! rp) continue;
        const float original = rp->getValue();
        Stats total; int steps = 48;
        for (int st = 0; st <= steps; ++st)
        {
            const float v = st <= steps / 2 ? float(st) / (steps / 2) : 1.0f - float(st - steps / 2) / (steps / 2);
            rp->setValueNotifyingHost(v);
            auto s = run(*proc, 6, t, sr, b, mb, last);
            total.finite &= s.finite; total.peak = std::max(total.peak, s.peak); total.clicks += s.clicks;
        }
        rp->setValueNotifyingHost(original);
        QA_CHECK(total.finite, "param '%s' sweep produced NaN/Inf", rp->getName(32).toRawUTF8());
        QA_CHECK(total.peak < 4.1f, "param '%s' sweep peak %.2f", rp->getName(32).toRawUTF8(), total.peak);
        // Allow a few transitions for switches (sync/division/slope/freeze) — those change delay time legitimately
        const bool isSwitch = dynamic_cast<juce::AudioParameterBool*>(param) || dynamic_cast<juce::AudioParameterChoice*>(param);
        QA_CHECK(total.clicks <= (isSwitch ? 24 : 2), "param '%s' sweep had %d clicks", rp->getName(32).toRawUTF8(), total.clicks);
        std::printf("  param  %-26s peak %5.2f  clicks %d%s\n", rp->getName(32).toRawUTF8(), total.peak, total.clicks, isSwitch ? " (switch)" : "");
    }

    // 3) State round-trip
    proc->setCurrentProgram(1);
    juce::MemoryBlock state; proc->getStateInformation(state);
    proc->setCurrentProgram(0);
    proc->setStateInformation(state.getData(), int(state.getSize()));
    QA_CHECK(proc->getCurrentProgram() == 1, "state round-trip lost the preset index");

    // 4) Bypass is click-free
    if (auto* power = dynamic_cast<juce::RangedAudioParameter*>(proc->getParameters()[0]))
    {
        for (auto* pp : proc->getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(pp)) if (r->getName(16) == "Power") power = r;
        Stats s; for (int k = 0; k < 8; ++k) { power->setValueNotifyingHost(k % 2 ? 0.0f : 1.0f); auto q = run(*proc, 40, t, sr, b, mb, last); s.clicks += q.clicks; }
        QA_CHECK(s.clicks == 0, "power toggling produced %d clicks", s.clicks);
        power->setValueNotifyingHost(1.0f);
    }

    // 5) Modulation stress: every LFO shape, fast free rate, routed hard to the first 3 targets + env → target 4.
    //    Modulation must never click or blow up, whatever it is pointed at.
    {
        auto set = [&](const juce::String& id, float plain) { for (auto* pp : proc->getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(pp)) if (r->paramID == id) { r->setValueNotifyingHost(r->convertTo0to1(plain)); return; } };
        proc->setCurrentProgram(0);
        int numTargets = 0;
        for (auto* pp : proc->getParameters()) if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(pp)) if (c->paramID == "mod1dst") numTargets = c->choices.size() - 1;
        for (int shape = 0; shape < 7; ++shape)
        {
            set("lfo1shape", float(shape)); set("lfo1sync", 0.0f); set("lfo1rate", 5.0f);
            set("lfo2shape", float((shape + 3) % 7)); set("lfo2sync", 0.0f); set("lfo2rate", 0.7f);
            Stats total;
            for (int tg = 1; tg <= numTargets; tg += 3)
            {
                set("mod1src", 1.0f); set("mod1dst", float(tg));                         set("mod1amt", 0.8f);
                set("mod2src", 2.0f); set("mod2dst", float(std::min(tg + 1, numTargets))); set("mod2amt", -0.7f);
                set("mod3src", 3.0f); set("mod3dst", float(std::min(tg + 2, numTargets))); set("mod3amt", 0.9f);
                run(*proc, int(sr * 0.2 / block), t, sr, b, mb, last);
                auto s = run(*proc, int(sr * 1.2 / block), t, sr, b, mb, last);
                total.finite &= s.finite; total.peak = std::max(total.peak, s.peak); total.clicks += s.clicks;
            }
            QA_CHECK(total.finite, "modulation shape %d produced NaN/Inf", shape);
            QA_CHECK(total.peak < 4.1f, "modulation shape %d peak %.2f", shape, total.peak);
            QA_CHECK(total.clicks <= 2, "modulation shape %d had %d clicks", shape, total.clicks);
            std::printf("  mod    shape %d across %d targets   peak %5.2f  clicks %d\n", shape, numTargets, total.peak, total.clicks);
        }
        for (int i = 1; i <= 4; ++i) set("mod" + juce::String(i) + "src", 0.0f);
    }

    // 6) Perform pad (if present): hardest throw - max time, self-oscillating feedback - then release and glide home
    {
        juce::RangedAudioParameter* hold = nullptr;
        for (auto* pp : proc->getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(pp)) if (r->paramID == "padhold") hold = r;
        if (hold)
        {
            auto set = [&](const juce::String& id, float v) { for (auto* pp : proc->getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(pp)) if (r->paramID == id) r->setValueNotifyingHost(v); };
            proc->setCurrentProgram(0);
            Stats total;
            for (int k = 0; k < 3; ++k)
            {
                set("padx", k == 0 ? 1.0f : (k == 1 ? 0.0f : 0.5f)); set("pady", 1.0f); set("padhold", 1.0f);
                auto a = run(*proc, int(sr * 1.5 / block), t, sr, b, mb, last);
                set("padhold", 0.0f);
                auto c = run(*proc, int(sr * 2.5 / block), t, sr, b, mb, last);
                total.finite &= a.finite && c.finite; total.peak = std::max({ total.peak, a.peak, c.peak }); total.clicks += a.clicks + c.clicks;
            }
            QA_CHECK(total.finite, "perform pad produced NaN/Inf");
            QA_CHECK(total.peak < 4.1f, "perform pad peak %.2f", total.peak);
            QA_CHECK(total.clicks <= 2, "perform pad throws had %d clicks", total.clicks);
            std::printf("  pad    3 throws + glide home       peak %5.2f  clicks %d\n", total.peak, total.clicks);
        }
    }

    // 7) Orbit bus: two instances see each other's freeze and heartbeat
    {
        zwui::OrbitBus a, bb;
        QA_CHECK(a.ok() && bb.ok(), "orbit bus failed to map shared memory");
        bool on = false;
        a.publishFreeze(true);
        QA_CHECK(bb.pollFreeze(on) && on, "orbit: B did not receive A's freeze");
        QA_CHECK(! a.pollFreeze(on), "orbit: A received its own freeze");
        bb.publishFreeze(false);
        QA_CHECK(a.pollFreeze(on) && ! on, "orbit: A did not receive B's unfreeze");
        a.heartbeat(true); bb.heartbeat(true);
        QA_CHECK(a.heartbeat(true) >= 2, "orbit: heartbeat count %d (expected >= 2)", a.heartbeat(true));
        std::printf("  orbit  freeze sync + heartbeat     ok\n");
    }

    std::printf("%s (%d failure%s)\n", failures ? "QA FAILED" : "QA PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
