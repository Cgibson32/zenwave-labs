#include "Presets.h"
#include "PluginProcessor.h"
#include <zenwave_ui/Modulation.h>
#include <zenwave_ui/ProcessorHelpers.h>

namespace Presets
{
    // name, category, inDb, depth, time, shapeAtk, shapeSus, duck, mix, outDb, lowX, highX, auto, subLock, grit,
    //   low{depth,gain,atk,rel}, mid{...}, high{...}, n1{on,freq,q,thr,range}, n2{...}
    static const Node off { false, 300.0f, 2.0f, 0.0f, -6.0f };
    static const Preset kPresets[] = {
        { "Init",                 "Init",      0.0f, 0.50f, 0.50f,  0.00f,  0.00f, 0.00f, 1.00f, 0.0f, 150.0f, 3000.0f, true,  true,  false, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, off, off },

        // --- Signature ---
        { "Bass Bus Control",     "Signature", 0.0f, 0.55f, 0.55f,  0.15f,  0.00f, 0.00f, 1.00f, 0.0f, 120.0f, 2500.0f, true,  true,  true,  { 0.6f, 0, 1.2f, 1.2f }, { 1.0f, 0.5f, 1, 1 }, { 0.8f, 0, 0.8f, 0.8f }, { true, 280.0f, 2.5f, 2.0f, -5.0f }, off },
        { "Vocal Glue",           "Signature", 0.0f, 0.40f, 0.60f,  0.00f,  0.10f, 0.00f, 1.00f, 0.0f, 180.0f, 4000.0f, true,  true,  false, { 0.4f, 0, 1, 1 }, { 1.0f, 0, 1, 1.2f }, { 0.9f, 0.5f, 0.7f, 0.8f }, { true, 3200.0f, 3.0f, 3.0f, -6.0f }, { true, 400.0f, 2.0f, 2.0f, -4.0f } },
        { "Drum Bus Punch",       "Signature", 0.0f, 0.45f, 0.40f,  0.45f, -0.15f, 0.00f, 1.00f, 0.0f, 110.0f, 4000.0f, true,  false, true,  { 0.8f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, { 0.9f, 0.5f, 0.6f, 0.8f }, off, off },
        { "Pump (Pattern)",       "Signature", 0.0f, 0.50f, 0.50f,  0.00f,  0.00f, 0.50f, 1.00f, 0.0f, 150.0f, 3000.0f, true,  true,  false, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, off, off },
        { "Master Glass",         "Signature", 0.0f, 0.25f, 0.70f,  0.05f,  0.05f, 0.00f, 1.00f, 0.0f, 100.0f, 5000.0f, true,  true,  false, { 0.7f, 0, 1.3f, 1.3f }, { 1.0f, 0, 1, 1 }, { 0.8f, 0.3f, 0.8f, 1.0f }, { true, 250.0f, 1.8f, 3.0f, -3.0f }, { true, 6500.0f, 2.5f, 3.0f, -3.0f } },

        // --- Bass ---
        { "OTT Classic",          "Bass",      0.0f, 0.80f, 0.45f,  0.00f,  0.00f, 0.00f, 1.00f, 0.0f, 150.0f, 3000.0f, false, false, false, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, off, off },
        { "Growl Upfront",        "Bass",      0.0f, 0.70f, 0.40f,  0.20f,  0.20f, 0.00f, 1.00f, 0.0f, 130.0f, 2800.0f, true,  true,  true,  { 0.5f, 0, 1, 1 }, { 1.0f, 1.0f, 0.8f, 0.8f }, { 1.0f, 1.0f, 0.6f, 0.7f }, { true, 600.0f, 2.0f, 4.0f, -4.0f }, off },
        { "Sub Lock Squash",      "Bass",      0.0f, 0.90f, 0.35f,  0.00f,  0.30f, 0.00f, 1.00f, 0.0f, 90.0f,  2200.0f, true,  true,  false, { 0.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, off, off },
        { "Reese Tamer",          "Bass",      0.0f, 0.55f, 0.65f,  0.00f,  0.00f, 0.00f, 1.00f, 0.0f, 140.0f, 3500.0f, true,  true,  false, { 0.6f, 0, 1.5f, 1.5f }, { 1.0f, 0, 1.2f, 1.2f }, { 0.7f, 0, 1, 1 }, { true, 350.0f, 2.5f, 1.0f, -8.0f }, { true, 1800.0f, 2.0f, 3.0f, -4.0f } },

        // --- Vocals / Leads ---
        { "De-Harsh Vox",         "Vocals",    0.0f, 0.30f, 0.60f,  0.00f,  0.00f, 0.00f, 1.00f, 0.0f, 200.0f, 4000.0f, true,  true,  false, { 0.3f, 0, 1, 1 }, { 0.8f, 0, 1, 1 }, { 1.0f, 0, 0.7f, 0.8f }, { true, 3200.0f, 3.5f, 1.0f, -8.0f }, { true, 7000.0f, 3.0f, 2.0f, -6.0f } },
        { "Lead Forward",         "Vocals",    0.0f, 0.50f, 0.45f,  0.30f,  0.10f, 0.00f, 1.00f, 0.0f, 250.0f, 3500.0f, true,  true,  true,  { 0.5f, 0, 1, 1 }, { 1.0f, 0.5f, 1, 1 }, { 1.0f, 0.5f, 0.7f, 0.8f }, off, off },

        // --- Drums ---
        { "Snare Snap",           "Drums",     0.0f, 0.35f, 0.30f,  0.70f, -0.30f, 0.00f, 1.00f, 0.0f, 180.0f, 3000.0f, true,  false, false, { 0.6f, 0, 1, 1 }, { 1.0f, 0, 0.8f, 0.8f }, { 1.0f, 0.5f, 0.6f, 0.6f }, off, off },
        { "Kick Weight",          "Drums",     0.0f, 0.40f, 0.60f,  0.20f,  0.30f, 0.00f, 1.00f, 0.0f, 80.0f,  1500.0f, true,  false, true,  { 1.0f, 0.5f, 1.2f, 1.4f }, { 0.8f, 0, 1, 1 }, { 0.5f, 0, 1, 1 }, off, off },
        { "Room Tail Pull",       "Drums",     0.0f, 0.50f, 0.50f, -0.10f, -0.60f, 0.00f, 1.00f, 0.0f, 150.0f, 3000.0f, true,  true,  false, { 0.8f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, off, off },

        // --- Sidechain / Rhythm ---
        { "Kick Duck (SC)",       "Rhythm",    0.0f, 0.30f, 0.50f,  0.00f,  0.00f, 0.00f, 1.00f, 0.0f, 150.0f, 3000.0f, true,  true,  false, { 0.5f, 0, 1, 1 }, { 0.8f, 0, 1, 1 }, { 0.8f, 0, 1, 1 }, off, off },
        { "Gate Chop",            "Rhythm",    0.0f, 0.30f, 0.50f,  0.00f,  0.00f, 0.50f, 1.00f, 0.0f, 150.0f, 3000.0f, true,  true,  false, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, { 1.0f, 0, 1, 1 }, off, off },

        // --- Mix / Master ---
        { "Bus Glue (Light)",     "Master",    0.0f, 0.20f, 0.70f,  0.00f,  0.00f, 0.00f, 1.00f, 0.0f, 120.0f, 5000.0f, true,  true,  false, { 0.8f, 0, 1.3f, 1.3f }, { 1.0f, 0, 1, 1 }, { 0.8f, 0, 0.9f, 1 }, off, off },
        { "Loud & Clear",         "Master",    0.0f, 0.45f, 0.55f,  0.10f,  0.10f, 0.00f, 1.00f, 1.0f, 100.0f, 4500.0f, true,  true,  true,  { 0.7f, 0, 1.2f, 1.2f }, { 1.0f, 0, 1, 1 }, { 0.9f, 0.5f, 0.8f, 0.9f }, { true, 300.0f, 2.0f, 3.0f, -3.0f }, off },
    };

    int count() { return int(std::size(kPresets)); }
    const Preset& get(int index) { return kPresets[juce::jlimit(0, count() - 1, index)]; }
    juce::String name(int index) { return get(index).name; }
    juce::String category(int index) { return get(index).category; }

    static void setParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float plainValue)
    {
        if (auto* p = apvts.getParameter(id)) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(plainValue)); p->endChangeGesture(); }
    }

    void apply(int index, juce::AudioProcessorValueTreeState& apvts)
    {
        const auto& p = get(index);
        setParam(apvts, ParamIDs::in, p.inDb); setParam(apvts, ParamIDs::depth, p.depth); setParam(apvts, ParamIDs::time, p.time);
        setParam(apvts, ParamIDs::shapeAtk, p.shapeAttack); setParam(apvts, ParamIDs::shapeSus, p.shapeSustain);
        setParam(apvts, ParamIDs::duck, p.duck); setParam(apvts, ParamIDs::mix, p.mix); setParam(apvts, ParamIDs::output, p.outDb);
        setParam(apvts, ParamIDs::lowX, p.lowX); setParam(apvts, ParamIDs::highX, p.highX);
        setParam(apvts, ParamIDs::autoMode, p.autoMode ? 1.0f : 0.0f); setParam(apvts, ParamIDs::subLock, p.subLock ? 1.0f : 0.0f); setParam(apvts, ParamIDs::grit, p.grit ? 1.0f : 0.0f);
        const Band* bands[3] = { &p.low, &p.mid, &p.high };
        for (int b = 0; b < 3; ++b)
        {
            setParam(apvts, ParamIDs::bandOn(b), 1.0f); setParam(apvts, ParamIDs::bandSolo(b), 0.0f);
            setParam(apvts, ParamIDs::bandDepth(b), bands[b]->depth); setParam(apvts, ParamIDs::bandGain(b), bands[b]->gainDb);
            setParam(apvts, ParamIDs::bandAtk(b), bands[b]->attack); setParam(apvts, ParamIDs::bandRel(b), bands[b]->release);
        }
        const Node* nodes[2] = { &p.n1, &p.n2 };
        const float nodeDef[4] = { 250.0f, 800.0f, 3000.0f, 8000.0f };
        for (int i = 0; i < 4; ++i)
        {
            const Node* nd = i < 2 ? nodes[i] : &off;
            setParam(apvts, ParamIDs::nodeOn(i), nd->on ? 1.0f : 0.0f);
            setParam(apvts, ParamIDs::nodeFreq(i), nd->on ? nd->freqHz : nodeDef[i]);
            setParam(apvts, ParamIDs::nodeQ(i), nd->q); setParam(apvts, ParamIDs::nodeThr(i), nd->thresh); setParam(apvts, ParamIDs::nodeRange(i), nd->range);
        }
        setParam(apvts, ParamIDs::scAmount, 0.0f); setParam(apvts, ParamIDs::scRelease, 150.0f);
        setParam(apvts, ParamIDs::power, 1.0f);

        // Motion (targets 1-based: Depth=1 Time Mix Duck Attack Sustain LowX HighX LowDepth MidDepth HighDepth N1Freq..N4Freq=15 N1Range..N4Range=19)
        const juce::String n(p.name);
        if (n == "Pump (Pattern)")  { zwui::setLfo(apvts, 0, 7, true, 10, 1.0f); zwui::setModSlot(apvts, 0, 1, 4, 0.5f);          // step lane drives DUCK: a pump that needs no routing
                                      for (int i = 0; i < 16; ++i) zwui::setParamPlain(apvts, zwui::ModIDs::step(0, i), (i % 4 == 0) ? 1.0f : (i % 4 == 1 ? 0.3f : (i % 4 == 2 ? -0.4f : -1.0f))); }
        if (n == "Gate Chop")       { zwui::setLfo(apvts, 0, 7, true, 9, 1.0f);  zwui::setModSlot(apvts, 0, 1, 4, 0.5f); zwui::setParamPlain(apvts, zwui::ModIDs::glide(0), 0.0f);
                                      for (int i = 0; i < 16; ++i) zwui::setParamPlain(apvts, zwui::ModIDs::step(0, i), (i % 2 == 0) ? -1.0f : 1.0f); }   // alternate open/closed
        if (n == "Kick Duck (SC)")  { setParam(apvts, ParamIDs::scAmount, 0.8f); setParam(apvts, ParamIDs::scRelease, 180.0f); }
        if (n == "Bass Bus Control"){ zwui::setEnv(apvts, 2.0f, 150.0f, 0.3f, 250.0f); zwui::setModSlot(apvts, 0, 3, 5, 0.25f); }   // hits get a little more attack
        if (n == "Drum Bus Punch")  { zwui::setEnv(apvts, 1.0f, 80.0f, 0.0f, 120.0f);  zwui::setModSlot(apvts, 0, 3, 1, -0.20f); }  // depth eases on each hit: punch survives
        if (n == "Master Glass")    { zwui::setLfo(apvts, 1, 6, true, 12, 0.1f);      zwui::setModSlot(apvts, 0, 2, 2, 0.10f); }    // timing breathes over 4 bars
    }
}
