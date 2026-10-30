#include "Presets.h"
#include "PluginProcessor.h"

namespace Presets
{
    // name, category, timeMs, sync, division(5 = 1/8), feedback, wobble, age, space, size, decay, width, spread, pingPong, mix, outputDb
    static const Preset kPresets[] = {
        // --- Init ---
        { "Init",                      "Init",      380.0f, false, 5, 0.45f, 0.30f, 0.40f, 0.35f, 0.60f, 0.60f, 1.00f,  0.00f, true,  0.35f, 0.0f },

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
        { "Self-Oscillate Riser",      "FX",        180.0f, false, 5, 1.05f, 0.40f, 0.50f, 0.40f, 0.60f, 0.60f, 1.00f,  0.00f, false, 0.80f, -6.0f },
        { "Infinite Freeze Pad",       "FX",        500.0f, false, 5, 1.00f, 0.30f, 0.30f, 1.00f, 1.00f, 1.00f, 1.40f,  0.00f, true,  1.00f, -3.0f },
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
    }
}
