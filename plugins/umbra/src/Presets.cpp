#include "Presets.h"
#include "PluginProcessor.h"

namespace Presets
{
    // name, category, preDelayMs, size, decay, dark, density, motion, gravity(0..1 → 40..300 Hz), early, duck, width, mix, outputDb, freeze
    static const Preset kPresets[] = {
        { "Init",                    "Init",     20.0f, 0.65f, 0.60f, 0.50f, 0.60f, 0.30f, 0.40f, 0.30f, 0.00f, 1.00f, 0.30f, 0.0f, false },

        // --- Bass music (designed so the sub never gets touched) ---
        { "Sub-Safe Drop Room",      "Bass",     12.0f, 0.45f, 0.40f, 0.55f, 0.70f, 0.20f, 0.55f, 0.40f, 0.35f, 1.00f, 0.22f, 0.0f, false },
        { "Growl Shadow",            "Bass",     18.0f, 0.55f, 0.50f, 0.70f, 0.65f, 0.25f, 0.60f, 0.25f, 0.45f, 1.10f, 0.28f, 0.0f, false },
        { "Halftime Cathedral",      "Bass",     35.0f, 0.85f, 0.75f, 0.60f, 0.60f, 0.35f, 0.50f, 0.20f, 0.30f, 1.20f, 0.32f, 0.0f, false },
        { "Riddim Slapback Space",   "Bass",      8.0f, 0.30f, 0.30f, 0.45f, 0.80f, 0.15f, 0.65f, 0.55f, 0.50f, 0.90f, 0.20f, 0.0f, false },
        { "Deep Dub Tunnel",         "Bass",     60.0f, 0.90f, 0.85f, 0.85f, 0.55f, 0.40f, 0.45f, 0.15f, 0.40f, 1.20f, 0.40f, 0.0f, false },
        { "Vocal Chop Ghost",        "Bass",     25.0f, 0.60f, 0.55f, 0.65f, 0.70f, 0.30f, 0.50f, 0.30f, 0.60f, 1.10f, 0.35f, 0.0f, false },
        { "Snare Dungeon",           "Bass",      5.0f, 0.50f, 0.35f, 0.75f, 0.85f, 0.10f, 0.70f, 0.60f, 0.20f, 1.00f, 0.25f, 0.0f, false },
        { "Pre-Drop Black Hole",     "Bass",     40.0f, 1.00f, 0.95f, 0.70f, 0.60f, 0.50f, 0.40f, 0.10f, 0.00f, 1.40f, 0.60f, 0.0f, false },

        // --- Dark spaces ---
        { "Umbra (Signature)",       "Dark",     30.0f, 0.75f, 0.70f, 0.80f, 0.65f, 0.35f, 0.45f, 0.25f, 0.20f, 1.15f, 0.35f, 0.0f, false },
        { "Pitch Black",             "Dark",     50.0f, 0.95f, 0.90f, 1.00f, 0.50f, 0.40f, 0.40f, 0.10f, 0.10f, 1.20f, 0.45f, 0.0f, false },
        { "Collapsed Star",          "Dark",     20.0f, 0.70f, 0.80f, 0.90f, 0.90f, 0.60f, 0.50f, 0.20f, 0.30f, 1.30f, 0.40f, 0.0f, false },
        { "Cold Hangar",             "Dark",     45.0f, 0.80f, 0.65f, 0.60f, 0.40f, 0.20f, 0.35f, 0.50f, 0.20f, 1.10f, 0.30f, 0.0f, false },
        { "Basement Ritual",         "Dark",     15.0f, 0.40f, 0.55f, 0.85f, 0.75f, 0.25f, 0.60f, 0.45f, 0.30f, 0.95f, 0.30f, 0.0f, false },

        // --- Mix utility (subtle, glue) ---
        { "Glue Plate (Dark)",       "Mix",      22.0f, 0.50f, 0.45f, 0.55f, 0.85f, 0.20f, 0.45f, 0.35f, 0.25f, 1.00f, 0.18f, 0.0f, false },
        { "Drum Bus Air (Low)",      "Mix",      10.0f, 0.35f, 0.30f, 0.50f, 0.90f, 0.10f, 0.55f, 0.60f, 0.40f, 1.05f, 0.14f, 0.0f, false },
        { "Lead Halo (Ducked)",      "Mix",      28.0f, 0.60f, 0.60f, 0.45f, 0.70f, 0.35f, 0.50f, 0.25f, 0.75f, 1.20f, 0.30f, 0.0f, false },
        { "Pad Bed",                 "Mix",      40.0f, 0.80f, 0.75f, 0.55f, 0.60f, 0.45f, 0.40f, 0.15f, 0.00f, 1.25f, 0.45f, 0.0f, false },

        // --- FX ---
        { "Freeze: Void Pad",        "FX",       30.0f, 1.00f, 1.00f, 0.60f, 0.70f, 0.50f, 0.40f, 0.00f, 0.00f, 1.40f, 1.00f, -3.0f, true },
        { "Freeze: Dark Drone",      "FX",       30.0f, 0.90f, 1.00f, 0.95f, 0.60f, 0.70f, 0.40f, 0.00f, 0.00f, 1.20f, 1.00f, -3.0f, true },
        { "Impact Bloom",            "FX",       80.0f, 1.00f, 0.90f, 0.65f, 0.50f, 0.40f, 0.45f, 0.30f, 0.00f, 1.30f, 0.70f, 0.0f, false },
        { "Reverse-Feel Swell",      "FX",      250.0f, 0.90f, 0.80f, 0.70f, 0.40f, 0.60f, 0.40f, 0.00f, 0.00f, 1.30f, 0.65f, 0.0f, false },
    };

    int count() { return int(std::size(kPresets)); }
    const Preset& get(int index) { return kPresets[juce::jlimit(0, count() - 1, index)]; }
    juce::String name(int index) { return get(index).name; }
    juce::String category(int index) { return get(index).category; }

    static void setParam(juce::AudioProcessorValueTreeState& apvts, const char* id, float plainValue)
    {
        if (auto* p = apvts.getParameter(id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(plainValue));
            p->endChangeGesture();
        }
    }

    void apply(int index, juce::AudioProcessorValueTreeState& apvts)
    {
        const auto& p = get(index);
        setParam(apvts, ParamIDs::preDelay, p.preDelayMs);
        setParam(apvts, ParamIDs::size,     p.size);
        setParam(apvts, ParamIDs::decay,    p.decay);
        setParam(apvts, ParamIDs::dark,     p.dark);
        setParam(apvts, ParamIDs::density,  p.density);
        setParam(apvts, ParamIDs::motion,   p.motion);
        setParam(apvts, ParamIDs::gravity,  p.gravity);
        setParam(apvts, ParamIDs::early,    p.early);
        setParam(apvts, ParamIDs::duck,     p.duck);
        setParam(apvts, ParamIDs::width,    p.width);
        setParam(apvts, ParamIDs::mix,      p.mix);
        setParam(apvts, ParamIDs::output,   p.outputDb);
        setParam(apvts, ParamIDs::freeze,   p.freeze ? 1.0f : 0.0f);
    }
}
