#include "Presets.h"
#include "PluginProcessor.h"
#include <zenwave_ui/Modulation.h>

namespace Presets
{
    // name, category, timeMs, sync, division(5 = 1/8), feedback, wobble, age, space, size, decay, width, spread, pingPong, mix, outputDb
    static const Preset kPresets[] = {
        // --- Init ---
        { "Init",                      "Init",      380.0f, false, 5, 0.45f, 0.30f, 0.40f, 0.35f, 0.60f, 0.60f, 1.00f,  0.00f, true,  0.35f, 0.0f },

        // --- Signature: engineered end-to-end (EQ, motion, dynamics) ---
        { "Zero-G Throw",              "Signature",   0.0f, true,  7, 0.55f, 0.22f, 0.50f, 0.45f, 0.70f, 0.65f, 1.20f,  0.10f, true,  1.00f, 0.0f },
        { "Tape Ghost Vox",            "Signature",   0.0f, true,  8, 0.42f, 0.35f, 0.70f, 0.30f, 0.55f, 0.50f, 1.00f,  0.00f, false, 0.30f, 0.0f },
        { "Dub Siren Pit",             "Signature", 420.0f, false, 5, 0.78f, 0.30f, 0.60f, 0.60f, 0.85f, 0.80f, 1.30f,  0.15f, true,  0.50f, 0.0f },
        { "Halftime Haze",             "Signature",   0.0f, true,  9, 0.50f, 0.40f, 0.45f, 0.55f, 0.90f, 0.75f, 1.40f,  0.05f, true,  0.35f, 0.0f },
        { "Riser Engine",              "Signature", 260.0f, false, 5, 1.02f, 0.30f, 0.55f, 0.50f, 0.80f, 0.70f, 1.50f,  0.20f, true,  0.80f, -4.0f },

        // --- Bass / Wonky / Halftime (vocal chops, lead stabs, fills) ---
        { "Wonky Throw",               "Bass",        0.0f, true,  5, 0.42f, 0.35f, 0.55f, 0.30f, 0.50f, 0.55f, 1.10f,  0.10f, true,  0.40f, 0.0f },
        { "Halftime Smear",            "Bass",        0.0f, true,  7, 0.55f, 0.25f, 0.60f, 0.50f, 0.70f, 0.65f, 1.00f,  0.00f, true,  0.45f, 0.0f },
        { "Riddim Slap",               "Bass",        0.0f, true,  2, 0.30f, 0.15f, 0.35f, 0.10f, 0.30f, 0.35f, 0.90f,  0.00f, false, 0.30f, 0.0f },
        { "Deep Dub Chamber",          "Bass",        0.0f, true,  7, 0.68f, 0.40f, 0.75f, 0.45f, 0.65f, 0.70f, 1.20f,  0.20f, true,  0.50f, 0.0f },
        { "Growl Tail",                "Bass",        0.0f, true,  5, 0.38f, 0.20f, 0.50f, 0.25f, 0.45f, 0.50f, 1.00f,  0.00f, true,  0.35f, 0.0f },
        { "Pre-Drop Vacuum",           "Bass",        0.0f, true,  8, 0.85f, 0.50f, 0.65f, 0.80f, 0.90f, 0.85f, 1.30f,  0.30f, true,  0.70f, 0.0f },
        { "Snare Throw (Dotted)",      "Bass",        0.0f, true,  4, 0.35f, 0.20f, 0.45f, 0.30f, 0.50f, 0.50f, 1.00f,  0.00f, true,  0.38f, 0.0f },

        // --- Space / Ambient (pads, intros, breakdowns) ---
        { "Orbit Decay",               "Space",     620.0f, false, 5, 0.72f, 0.45f, 0.50f, 0.85f, 0.90f, 0.85f, 1.30f,  0.25f, true,  0.55f, 0.0f },
        { "Event Horizon",             "Space",     900.0f, false, 5, 0.92f, 0.30f, 0.40f, 1.00f, 1.00f, 0.95f, 1.40f,  0.15f, true,  0.65f, 0.0f },
        { "Nebula Wash",               "Space",     450.0f, false, 5, 0.60f, 0.55f, 0.35f, 0.95f, 0.85f, 0.80f, 1.20f,  0.00f, true,  0.60f, 0.0f },
        { "Satellite Ping",            "Space",       0.0f, true,  2, 0.50f, 0.10f, 0.20f, 0.60f, 0.70f, 0.60f, 1.50f,  0.40f, true,  0.45f, 0.0f },
        { "Cold Vacuum (Dark)",        "Space",     780.0f, false, 5, 0.75f, 0.35f, 0.85f, 0.90f, 0.95f, 0.90f, 1.00f,  0.00f, false, 0.60f, 0.0f },

        // --- Tape / Lo-fi (drums, keys, guitars, vocals) ---
        { "Worn Cassette",             "Tape",      340.0f, false, 5, 0.40f, 0.65f, 0.80f, 0.20f, 0.40f, 0.45f, 0.95f,  0.05f, false, 0.35f, 0.0f },
        { "Dusty Slapback",            "Tape",       95.0f, false, 5, 0.12f, 0.25f, 0.70f, 0.05f, 0.30f, 0.30f, 0.80f,  0.00f, false, 0.30f, 0.0f },
        { "Broken Walkman",            "Tape",      410.0f, false, 5, 0.55f, 1.00f, 0.90f, 0.30f, 0.50f, 0.55f, 1.10f,  0.15f, true,  0.45f, 0.0f },
        { "Vocal Tape Echo",           "Tape",        0.0f, true,  7, 0.45f, 0.30f, 0.60f, 0.35f, 0.55f, 0.55f, 1.10f,  0.00f, true,  0.32f, 0.0f },
        { "Hazy Keys",                 "Tape",        0.0f, true,  5, 0.50f, 0.50f, 0.70f, 0.55f, 0.60f, 0.60f, 1.20f,  0.10f, true,  0.40f, 0.0f },

        // --- Rhythmic (synced, tighter, mix-ready) ---
        { "1/8 Pong Tight",            "Rhythmic",    0.0f, true,  5, 0.40f, 0.10f, 0.25f, 0.10f, 0.30f, 0.30f, 1.20f,  0.00f, true,  0.35f, 0.0f },
        { "Dotted Lead Bounce",        "Rhythmic",    0.0f, true,  7, 0.48f, 0.15f, 0.30f, 0.20f, 0.40f, 0.40f, 1.10f,  0.00f, true,  0.40f, 0.0f },
        { "Triplet Shuffle",           "Rhythmic",    0.0f, true,  3, 0.45f, 0.20f, 0.35f, 0.25f, 0.45f, 0.45f, 1.00f,  0.00f, true,  0.38f, 0.0f },
        { "Quarter Note Canyon",       "Rhythmic",    0.0f, true,  8, 0.60f, 0.20f, 0.40f, 0.50f, 0.75f, 0.70f, 1.20f,  0.10f, true,  0.45f, 0.0f },

        // --- FX / Sound Design ---
        { "Self-Oscillate Riser",      "FX",        180.0f, false, 5, 1.08f, 0.40f, 0.60f, 0.40f, 0.60f, 0.60f, 1.00f,  0.00f, false, 0.85f, 0.0f },
        { "Infinite Freeze Pad",       "FX",        500.0f, false, 5, 0.90f, 0.30f, 0.20f, 1.00f, 1.00f, 1.00f, 1.40f,  0.00f, true,  1.00f, -3.0f },
        { "Transition Sweep",          "FX",        260.0f, false, 5, 0.90f, 0.70f, 0.60f, 0.70f, 0.80f, 0.75f, 1.50f,  0.50f, true,  0.75f, -3.0f },
        { "Impact Tail",               "FX",      1200.0f, false, 5, 0.70f, 0.20f, 0.45f, 0.90f, 1.00f, 0.90f, 1.30f,  0.00f, true,  0.70f, 0.0f },
        { "Alien Transmission",        "FX",         60.0f, false, 5, 0.88f, 0.90f, 0.75f, 0.30f, 0.40f, 0.50f, 1.50f, -0.60f, true,  0.60f, -3.0f },
    };

    int count() { return int(std::size(kPresets)); }
    const Preset& get(int index) { return kPresets[juce::jlimit(0, count() - 1, index)]; }
    juce::String name(int index) { return get(index).name; }
    juce::String category(int index) { return get(index).category; }

    static void setParam(juce::AudioProcessorValueTreeState& apvts, const char* id, float plainValue)
    {
        if (auto* p = apvts.getParameter(id))
        {
            const float norm = p->convertTo0to1(plainValue);
            p->beginChangeGesture();
            p->setValueNotifyingHost(norm);
            p->endChangeGesture();
        }
    }

    void apply(int index, juce::AudioProcessorValueTreeState& apvts)
    {
        const auto& p = get(index);
        setParam(apvts, ParamIDs::time,       p.timeMs > 0.0f ? p.timeMs : 380.0f);
        setParam(apvts, ParamIDs::sync,       p.sync ? 1.0f : 0.0f);
        setParam(apvts, ParamIDs::division,   float(p.division));
        setParam(apvts, ParamIDs::feedback,   p.feedback);
        setParam(apvts, ParamIDs::wowFlutter, p.wobble);
        setParam(apvts, ParamIDs::age,        p.age);
        setParam(apvts, ParamIDs::space,      p.space);
        setParam(apvts, ParamIDs::size,       p.size);
        setParam(apvts, ParamIDs::decay,      p.decay);
        setParam(apvts, ParamIDs::width,      p.width);
        setParam(apvts, ParamIDs::spread,     p.spread);
        setParam(apvts, ParamIDs::pingPong,   p.pingPong ? 1.0f : 0.0f);
        setParam(apvts, ParamIDs::freeze,     (juce::String(p.name) == "Infinite Freeze Pad") ? 1.0f : 0.0f);
        setParam(apvts, ParamIDs::mix,        p.mix);
        setParam(apvts, ParamIDs::output,     p.outputDb);
        // Newer parameters not stored in the preset table: deterministic defaults
        setParam(apvts, ParamIDs::power,      1.0f);
        setParam(apvts, ParamIDs::wobbleSync, 0.0f);
        setParam(apvts, ParamIDs::wobbleDiv,  10.0f);
        setParam(apvts, ParamIDs::wobbleRate, 0.45f);
        setParam(apvts, ParamIDs::lowCut,     20.0f);
        setParam(apvts, ParamIDs::highCut,    20000.0f);
        setParam(apvts, ParamIDs::lowSlope,   1.0f);
        setParam(apvts, ParamIDs::highSlope,  1.0f);
        setParam(apvts, ParamIDs::tilt,       0.0f);
        setParam(apvts, ParamIDs::color,      juce::String(p.name) == "Deep Dub Chamber" ? -0.45f : (juce::String(p.name) == "Alien Transmission" ? 0.6f : 0.0f));

        setParam(apvts, zwui::FreezeQuantizer::paramId, 0.0f);
        // Factory motion (targets 1-based: Time=1 Feedback Wobble Age Space Size Decay Width Spread Mix WobbleRate LowCut HighCut Tilt Color=15)
        const juce::String n(p.name);
        auto eq = [&](float lc, int ls, float hc, int hs, float tl) { setParam(apvts, ParamIDs::lowCut, lc); setParam(apvts, ParamIDs::lowSlope, float(ls)); setParam(apvts, ParamIDs::highCut, hc); setParam(apvts, ParamIDs::highSlope, float(hs)); setParam(apvts, ParamIDs::tilt, tl); };
        // Signature set
        if (n == "Zero-G Throw")   { eq(180.0f, 2, 9000.0f, 1, -0.1f); setParam(apvts, ParamIDs::color, -0.2f);
                                     zwui::setEnv(apvts, 2.0f, 250.0f, 0.0f, 300.0f); zwui::setModSlot(apvts, 0, 3, 2, -0.25f);          // each hit briefly lowers feedback: throws never pile up
                                     zwui::setLfo(apvts, 1, 6, true, 11, 0.1f);       zwui::setModSlot(apvts, 1, 2, 8, 0.15f); }          // slow drift on width
        if (n == "Tape Ghost Vox") { eq(250.0f, 1, 6000.0f, 1, -0.2f); setParam(apvts, ParamIDs::color, -0.15f); setParam(apvts, ParamIDs::wobbleRate, 0.3f);
                                     zwui::setLfo(apvts, 0, 0, true, 10, 0.1f);       zwui::setModSlot(apvts, 0, 1, 11, 0.20f);
                                     zwui::setEnv(apvts, 5.0f, 120.0f, 1.0f, 400.0f); zwui::setModSlot(apvts, 1, 3, 10, -0.30f); }        // mix ducks while the vocal sings, swells between phrases
        if (n == "Dub Siren Pit")  { eq(120.0f, 2, 5000.0f, 1, -0.3f); setParam(apvts, ParamIDs::color, -0.5f);
                                     zwui::setLfo(apvts, 0, 6, true, 12, 0.1f);       zwui::setModSlot(apvts, 0, 1, 1, 0.06f);           // the tape drifts over four bars
                                     zwui::setLfo(apvts, 1, 0, true, 11, 0.1f);       zwui::setModSlot(apvts, 1, 2, 15, -0.20f); }       // colour breathes darker/lighter
        if (n == "Halftime Haze")  { eq(150.0f, 2, 8000.0f, 1, 0.0f); setParam(apvts, ParamIDs::color, -0.1f);
                                     zwui::setEnv(apvts, 10.0f, 400.0f, 0.3f, 600.0f); zwui::setModSlot(apvts, 0, 3, 5, 0.30f);          // hits open the space
                                     zwui::setLfo(apvts, 1, 6, true, 11, 0.1f);        zwui::setModSlot(apvts, 1, 2, 9, 0.25f); }        // spread wanders
        if (n == "Riser Engine")   { eq(60.0f, 2, 14000.0f, 1, 0.1f); setParam(apvts, zwui::FreezeQuantizer::paramId, 2.0f);
                                     zwui::setLfo(apvts, 0, 2, true, 12, 0.1f);       zwui::setModSlot(apvts, 0, 1, 12, 0.60f);          // 4-bar saw sweeps the low cut up
                                     zwui::setModSlot(apvts, 1, 1, 8, 0.20f);         zwui::setModSlot(apvts, 2, 1, 3, 0.30f); }         // ...and widens / wobbles as it climbs
        if (n == "Nebula Wash")          { zwui::setLfo(apvts, 1, 6, true, 11, 0.1f);  zwui::setModSlot(apvts, 0, 2, 8, 0.25f);  zwui::setModSlot(apvts, 1, 2, 15, -0.20f); }
        if (n == "Alien Transmission")   { zwui::setLfo(apvts, 0, 5, true, 5, 1.0f);   zwui::setLfo(apvts, 1, 0, false, 10, 0.3f);
                                           zwui::setModSlot(apvts, 0, 1, 1, 0.12f);  zwui::setModSlot(apvts, 1, 2, 9, 0.50f); }
        if (n == "Broken Walkman")       { zwui::setLfo(apvts, 0, 6, false, 8, 0.9f);  zwui::setModSlot(apvts, 0, 1, 11, 0.30f); zwui::setModSlot(apvts, 1, 1, 4, 0.15f); }
        if (n == "Pre-Drop Vacuum")      { zwui::setModSlot(apvts, 0, 3, 2, 0.25f);   zwui::setModSlot(apvts, 1, 3, 5, 0.30f); }
        if (n == "Transition Sweep")     { zwui::setLfo(apvts, 0, 3, true, 11, 0.1f);  zwui::setModSlot(apvts, 0, 1, 12, 0.70f); zwui::setModSlot(apvts, 1, 1, 8, 0.20f); }
        if (n == "Orbit Decay")          { zwui::setLfo(apvts, 1, 0, true, 10, 0.1f);  zwui::setModSlot(apvts, 0, 2, 9, 0.35f); }
        if (n == "Hazy Keys")            { zwui::setLfo(apvts, 1, 6, true, 11, 0.1f);  zwui::setModSlot(apvts, 0, 2, 3, 0.20f);  zwui::setModSlot(apvts, 1, 2, 15, -0.15f); }
    }
}
