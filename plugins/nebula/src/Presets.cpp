#include "Presets.h"
#include "PluginProcessor.h"
#include <zenwave_ui/Modulation.h>

namespace Presets
{
    // tables: 0 Glass 1 Voice 2 Metal 3 Grain 4 Dune 5 Halo 6 Ion 7 Void
    // Osc: table, position, level, octave, semis, fine, unison, spread, width, on
    // then: cutoff, res, filterMode, envAmt, drive, drift, glide | ampA D S R | modA D S R | spaceMix size decay dark | macros warmth air movement space grit width attack release
    static const Preset kPresets[] = {
        { "Init",             "Init",       { 0, 0.3f, 0.8f, 0, 0, 0.0f, 3, 0.25f, 0.7f, true }, { 5, 0.5f, 0.5f, 0, 0, 6.0f, 2, 0.25f, 0.7f, true },
          9000.0f, 0.2f, 0, 0.2f, 0.0f, 0.25f, 0.0f, 20.0f, 300.0f, 0.8f, 600.0f, 10.0f, 400.0f, 0.3f, 400.0f, 0.25f, 0.7f, 0.6f, 0.5f, 0.3f, 0.3f, 0.2f, 0.4f, 0.0f, 0.6f, 0.5f, 0.5f },

        // --- Atmosphere (background layers that sit behind everything) ---
        { "Event Horizon Pad", "Atmosphere",{ 0, 0.55f, 0.8f, 0, 0, 0.0f, 6, 0.35f, 0.9f, true }, { 5, 0.7f, 0.6f, 1, 0, -5.0f, 4, 0.3f, 0.9f, true },
          4500.0f, 0.15f, 0, 0.3f, 0.0f, 0.4f, 0.0f, 900.0f, 1500.0f, 0.9f, 2500.0f, 2000.0f, 2000.0f, 0.5f, 2000.0f, 0.5f, 0.95f, 0.85f, 0.6f, 0.45f, 0.4f, 0.4f, 0.6f, 0.0f, 0.8f, 0.6f, 0.6f },
        { "Cold Nebula",      "Atmosphere", { 3, 0.35f, 0.6f, 0, 0, 0.0f, 5, 0.4f, 1.0f, true }, { 0, 0.2f, 0.5f, 1, 7, 4.0f, 3, 0.2f, 0.8f, true },
          3200.0f, 0.25f, 0, 0.2f, 0.0f, 0.5f, 0.0f, 1500.0f, 2000.0f, 0.85f, 3000.0f, 1000.0f, 3000.0f, 0.4f, 3000.0f, 0.55f, 1.0f, 0.9f, 0.7f, 0.5f, 0.3f, 0.5f, 0.65f, 0.0f, 0.9f, 0.65f, 0.65f },
        { "Solar Wind",       "Atmosphere", { 6, 0.3f, 0.55f, 0, 0, 0.0f, 7, 0.5f, 1.0f, true }, { 7, 0.6f, 0.7f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          2400.0f, 0.3f, 0, 0.5f, 0.1f, 0.3f, 0.0f, 600.0f, 1200.0f, 0.8f, 1800.0f, 500.0f, 2500.0f, 0.2f, 1500.0f, 0.45f, 0.9f, 0.8f, 0.5f, 0.6f, 0.2f, 0.6f, 0.55f, 0.1f, 0.85f, 0.55f, 0.55f },
        { "Choir of Glass",   "Atmosphere", { 1, 0.4f, 0.7f, 0, 0, 0.0f, 5, 0.3f, 0.9f, true }, { 0, 0.8f, 0.5f, 1, 0, 7.0f, 4, 0.25f, 0.9f, true },
          6000.0f, 0.1f, 0, 0.2f, 0.0f, 0.35f, 0.0f, 400.0f, 1000.0f, 0.9f, 1800.0f, 300.0f, 2000.0f, 0.4f, 1500.0f, 0.5f, 0.9f, 0.85f, 0.4f, 0.3f, 0.5f, 0.5f, 0.6f, 0.0f, 0.75f, 0.55f, 0.6f },

        // --- Keys ---
        { "Glass Keys",       "Keys",       { 0, 0.25f, 0.85f, 0, 0, 0.0f, 2, 0.15f, 0.5f, true }, { 5, 0.3f, 0.35f, 1, 0, 3.0f, 1, 0.0f, 0.0f, true },
          7000.0f, 0.15f, 0, 0.6f, 0.0f, 0.15f, 0.0f, 3.0f, 900.0f, 0.3f, 700.0f, 2.0f, 600.0f, 0.1f, 500.0f, 0.3f, 0.6f, 0.5f, 0.4f, 0.3f, 0.3f, 0.15f, 0.4f, 0.0f, 0.4f, 0.3f, 0.5f },
        { "Dusty Rhodes",     "Keys",       { 2, 0.15f, 0.8f, 0, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 0, 0.1f, 0.5f, 0, 0, -4.0f, 2, 0.1f, 0.4f, true },
          3500.0f, 0.2f, 0, 0.7f, 0.15f, 0.2f, 0.0f, 2.0f, 1200.0f, 0.4f, 900.0f, 1.0f, 500.0f, 0.0f, 400.0f, 0.2f, 0.5f, 0.45f, 0.6f, 0.5f, 0.2f, 0.2f, 0.3f, 0.2f, 0.35f, 0.3f, 0.5f },
        { "Alien Keys",       "Keys",       { 2, 0.7f, 0.75f, 0, 0, 0.0f, 3, 0.3f, 0.7f, true }, { 6, 0.4f, 0.4f, 0, 7, 0.0f, 2, 0.2f, 0.6f, true },
          5000.0f, 0.35f, 0, 0.5f, 0.1f, 0.3f, 0.0f, 5.0f, 800.0f, 0.5f, 900.0f, 3.0f, 700.0f, 0.2f, 600.0f, 0.35f, 0.7f, 0.6f, 0.5f, 0.3f, 0.4f, 0.4f, 0.5f, 0.15f, 0.6f, 0.35f, 0.5f },

        // --- Plucks ---
        { "Halo Pluck",       "Plucks",     { 5, 0.2f, 0.85f, 0, 0, 0.0f, 3, 0.2f, 0.8f, true }, { 0, 0.5f, 0.4f, 1, 0, 5.0f, 2, 0.2f, 0.8f, true },
          2500.0f, 0.3f, 0, 0.8f, 0.0f, 0.2f, 0.0f, 1.0f, 500.0f, 0.0f, 500.0f, 1.0f, 350.0f, 0.0f, 300.0f, 0.35f, 0.6f, 0.55f, 0.4f, 0.3f, 0.4f, 0.3f, 0.5f, 0.0f, 0.6f, 0.2f, 0.4f },
        { "Ion Spark",        "Plucks",     { 6, 0.6f, 0.8f, 0, 0, 0.0f, 2, 0.15f, 0.6f, true }, { 4, 0.1f, 0.4f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1800.0f, 0.45f, 0, 0.9f, 0.2f, 0.1f, 0.0f, 1.0f, 350.0f, 0.0f, 300.0f, 0.5f, 250.0f, 0.0f, 250.0f, 0.2f, 0.4f, 0.4f, 0.4f, 0.4f, 0.1f, 0.2f, 0.3f, 0.3f, 0.5f, 0.15f, 0.35f },

        // --- Leads ---
        { "Dune Lead",        "Leads",      { 4, 0.2f, 0.9f, 0, 0, 0.0f, 4, 0.3f, 0.5f, true }, { 4, 0.7f, 0.5f, -1, 0, -3.0f, 1, 0.0f, 0.0f, true },
          6000.0f, 0.25f, 0, 0.4f, 0.25f, 0.15f, 60.0f, 8.0f, 400.0f, 0.7f, 300.0f, 5.0f, 300.0f, 0.2f, 300.0f, 0.2f, 0.5f, 0.5f, 0.4f, 0.25f, 0.4f, 0.3f, 0.3f, 0.3f, 0.5f, 0.4f, 0.4f },
        { "Siren Call",       "Leads",      { 1, 0.6f, 0.9f, 0, 0, 0.0f, 3, 0.2f, 0.4f, true }, { 5, 0.4f, 0.35f, 1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          4000.0f, 0.4f, 0, 0.5f, 0.15f, 0.2f, 120.0f, 20.0f, 500.0f, 0.8f, 400.0f, 10.0f, 600.0f, 0.3f, 400.0f, 0.3f, 0.6f, 0.6f, 0.5f, 0.3f, 0.3f, 0.5f, 0.4f, 0.15f, 0.4f, 0.45f, 0.45f },

        // --- Bass ---
        { "Void Sub",         "Bass",       { 7, 0.2f, 1.0f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true }, { 4, 0.0f, 0.3f, 0, 0, 0.0f, 1, 0.0f, 0.0f, true },
          900.0f, 0.1f, 0, 0.5f, 0.1f, 0.0f, 40.0f, 2.0f, 300.0f, 0.9f, 200.0f, 1.0f, 300.0f, 0.0f, 200.0f, 0.0f, 0.3f, 0.3f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.3f, 0.3f },
        { "Ion Growl",        "Bass",       { 6, 0.5f, 0.9f, -1, 0, 0.0f, 2, 0.1f, 0.3f, true }, { 7, 0.3f, 0.7f, -1, 0, 0.0f, 1, 0.0f, 0.0f, true },
          1500.0f, 0.5f, 0, 0.7f, 0.4f, 0.1f, 50.0f, 3.0f, 400.0f, 0.8f, 250.0f, 2.0f, 500.0f, 0.3f, 250.0f, 0.1f, 0.4f, 0.4f, 0.5f, 0.5f, 0.0f, 0.3f, 0.1f, 0.5f, 0.2f, 0.3f, 0.3f },
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
        const Osc* oscs[2] = { &p.a, &p.b };
        for (int o = 0; o < 2; ++o)
        {
            setParam(apvts, ParamIDs::osc(o, "table"), float(oscs[o]->table)); setParam(apvts, ParamIDs::osc(o, "position"), oscs[o]->position);
            setParam(apvts, ParamIDs::osc(o, "level"), oscs[o]->level); setParam(apvts, ParamIDs::osc(o, "octave"), float(oscs[o]->octave));
            setParam(apvts, ParamIDs::osc(o, "semis"), float(oscs[o]->semis)); setParam(apvts, ParamIDs::osc(o, "fine"), oscs[o]->fine);
            setParam(apvts, ParamIDs::osc(o, "unison"), float(oscs[o]->unison)); setParam(apvts, ParamIDs::osc(o, "spread"), oscs[o]->spread);
            setParam(apvts, ParamIDs::osc(o, "width"), oscs[o]->width); setParam(apvts, ParamIDs::osc(o, "on"), oscs[o]->on ? 1.0f : 0.0f);
        }
        setParam(apvts, ParamIDs::cutoff, p.cutoff); setParam(apvts, ParamIDs::res, p.res); setParam(apvts, ParamIDs::filterMode, float(p.filterMode));
        setParam(apvts, ParamIDs::envAmt, p.envAmt); setParam(apvts, ParamIDs::keyTrack, 0.5f); setParam(apvts, ParamIDs::drive, p.drive);
        setParam(apvts, ParamIDs::drift, p.drift); setParam(apvts, ParamIDs::glide, p.glideMs);
        setParam(apvts, ParamIDs::ampA, p.ampA); setParam(apvts, ParamIDs::ampD, p.ampD); setParam(apvts, ParamIDs::ampS, p.ampS); setParam(apvts, ParamIDs::ampR, p.ampR);
        setParam(apvts, ParamIDs::modA, p.modA); setParam(apvts, ParamIDs::modD, p.modD); setParam(apvts, ParamIDs::modS, p.modS); setParam(apvts, ParamIDs::modR, p.modR);
        setParam(apvts, ParamIDs::spaceMix, p.spaceMix); setParam(apvts, ParamIDs::spaceSize, p.spaceSize); setParam(apvts, ParamIDs::spaceDecay, p.spaceDecay); setParam(apvts, ParamIDs::spaceDark, p.spaceDark);
        setParam(apvts, ParamIDs::mWarmth, p.warmth); setParam(apvts, ParamIDs::mAir, p.air); setParam(apvts, ParamIDs::mMovement, p.movement); setParam(apvts, ParamIDs::mSpace, p.space);
        setParam(apvts, ParamIDs::mGrit, p.grit); setParam(apvts, ParamIDs::mWidth, p.width); setParam(apvts, ParamIDs::mAttack, p.attack); setParam(apvts, ParamIDs::mRelease, p.release);
        setParam(apvts, ParamIDs::carveOn, 1.0f);
        for (int i = 0; i < nebula::kCarveBands; ++i) setParam(apvts, ParamIDs::carveBandOn(i), 0.0f);
        setParam(apvts, ParamIDs::duck, 0.0f); setParam(apvts, ParamIDs::duckRelease, 250.0f);
        // FX page defaults
        const float eqDef[4] = { 120.0f, 500.0f, 2500.0f, 9000.0f }; const int eqTypeDef[4] = { 1, 0, 0, 2 };
        for (int i = 0; i < 4; ++i) { setParam(apvts, ParamIDs::eqOn(i), 0.0f); setParam(apvts, ParamIDs::eqType(i), float(eqTypeDef[i])); setParam(apvts, ParamIDs::eqFreq(i), eqDef[i]); setParam(apvts, ParamIDs::eqGain(i), 0.0f); setParam(apvts, ParamIDs::eqQ(i), 1.0f); }
        setParam(apvts, ParamIDs::satOn, 0.0f); setParam(apvts, ParamIDs::satType, 0.0f); setParam(apvts, ParamIDs::satDrive, 0.3f); setParam(apvts, ParamIDs::satTone, 0.0f); setParam(apvts, ParamIDs::satBias, 0.0f); setParam(apvts, ParamIDs::satLow, 20.0f); setParam(apvts, ParamIDs::satOut, 0.0f); setParam(apvts, ParamIDs::satMix, 1.0f);
        setParam(apvts, ParamIDs::glueOn, 0.0f); setParam(apvts, ParamIDs::glueDepth, 0.5f); setParam(apvts, ParamIDs::glueTime, 0.5f); setParam(apvts, ParamIDs::glueUp, 1.0f); setParam(apvts, ParamIDs::glueDown, 1.0f); setParam(apvts, ParamIDs::glueMix, 1.0f); setParam(apvts, ParamIDs::glueOut, 0.0f);
        setParam(apvts, ParamIDs::echoWobble, 0.0f); setParam(apvts, ParamIDs::echoWidth, 0.66f);
        setParam(apvts, ParamIDs::spaceOn, 1.0f); setParam(apvts, ParamIDs::spacePre, 0.0f); setParam(apvts, ParamIDs::spaceLow, 90.0f); setParam(apvts, ParamIDs::spaceShimmer, 0.0f);
        setParam(apvts, ParamIDs::echoOn, 0.0f); setParam(apvts, ParamIDs::echoSync, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoTime, 375.0f);
        setParam(apvts, ParamIDs::echoFb, 0.4f); setParam(apvts, ParamIDs::echoMix, 0.25f); setParam(apvts, ParamIDs::echoTone, 0.5f); setParam(apvts, ParamIDs::echoPP, 1.0f);
        setParam(apvts, ParamIDs::power, 1.0f);

        // carve + motion per preset (targets 1-based: PosA=1 PosB Cutoff Res Drive LevelA LevelB Space Warmth Air Movement Grit Width Drift Duck=15)
        const juce::String n(p.name);
        auto band = [&](int i, float centre, float widthOct, float gain) { setParam(apvts, ParamIDs::carveBandOn(i), 1.0f); setParam(apvts, ParamIDs::carveBandCentre(i), centre); setParam(apvts, ParamIDs::carveBandWidth(i), widthOct); setParam(apvts, ParamIDs::carveBandGain(i), gain); };
        if (n == "Choir of Glass")    { band(0, 2.0f, 0.4f, 0.2f); band(1, 3.0f, 0.3f, 0.3f); band(2, 9.0f, 0.5f, 2.0f);
                                        zwui::setLfo(apvts, 0, 0, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.20f); zwui::setLfo(apvts, 1, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 1, 2, 2, 0.25f); }
        if (n == "Event Horizon Pad") { zwui::setLfo(apvts, 0, 0, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 1, 0.30f); zwui::setLfo(apvts, 1, 6, true, 11, 0.1f); zwui::setModSlot(apvts, 1, 2, 3, 0.15f); }
        if (n == "Cold Nebula")       { band(0, 1.0f, 0.3f, 1.0f); band(1, 6.0f, 1.0f, 0.3f); zwui::setLfo(apvts, 0, 6, true, 12, 0.1f); zwui::setModSlot(apvts, 0, 1, 2, 0.35f); }
        if (n == "Solar Wind")        { band(0, 4.0f, 0.8f, 0.15f); band(1, 24.0f, 0.6f, 2.5f); zwui::setLfo(apvts, 0, 1, true, 11, 0.1f); zwui::setModSlot(apvts, 0, 1, 3, 0.30f); zwui::setModSlot(apvts, 1, 1, 13, 0.20f); }
        if (n == "Alien Keys")        { band(0, 2.0f, 0.3f, 0.0f); band(1, 7.0f, 0.4f, 3.0f); band(2, 11.0f, 0.3f, 2.5f); }
        if (n == "Siren Call")        { band(0, 3.0f, 0.5f, 2.5f); band(1, 5.0f, 0.4f, 0.3f); zwui::setLfo(apvts, 0, 0, false, 8, 5.5f); zwui::setModSlot(apvts, 0, 1, 1, 0.08f); }
        if (n == "Ion Growl")         { band(0, 3.0f, 0.4f, 3.0f); band(1, 8.0f, 0.6f, 0.2f); zwui::setLfo(apvts, 0, 7, true, 10, 1.0f); zwui::setModSlot(apvts, 0, 1, 3, 0.40f); zwui::setModSlot(apvts, 1, 1, 1, 0.30f); }
        if (n == "Dune Lead")         { zwui::setEnv(apvts, 2.0f, 200.0f, 0.3f, 200.0f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.35f); setParam(apvts, ParamIDs::echoMix, 0.2f); setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.35f); }
        if (n == "Choir of Glass")    { setParam(apvts, ParamIDs::spaceShimmer, 1.0f); setParam(apvts, ParamIDs::spacePre, 40.0f); }
        if (n == "Cold Nebula")       { setParam(apvts, ParamIDs::spacePre, 60.0f); setParam(apvts, ParamIDs::spaceLow, 160.0f); }
        if (n == "Dusty Rhodes")      { setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 1.0f); setParam(apvts, ParamIDs::satDrive, 0.3f); setParam(apvts, ParamIDs::satTone, -0.25f); setParam(apvts, ParamIDs::satBias, 0.3f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoWobble, 0.45f); setParam(apvts, ParamIDs::echoMix, 0.15f); setParam(apvts, ParamIDs::echoTone, 0.3f); }
        if (n == "Siren Call")        { setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 5.0f); setParam(apvts, ParamIDs::satDrive, 0.4f); setParam(apvts, ParamIDs::satLow, 150.0f); setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 6.0f); setParam(apvts, ParamIDs::echoFb, 0.45f); setParam(apvts, ParamIDs::echoMix, 0.22f); setParam(apvts, ParamIDs::echoWidth, 1.0f); }
        if (n == "Void Sub")          { setParam(apvts, ParamIDs::spaceOn, 0.0f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.35f); setParam(apvts, ParamIDs::glueUp, 0.3f); setParam(apvts, ParamIDs::glueTime, 0.8f); }
        if (n == "Glass Keys")        { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 4.0f); setParam(apvts, ParamIDs::echoFb, 0.3f); setParam(apvts, ParamIDs::echoMix, 0.18f); setParam(apvts, ParamIDs::eqOn(0), 1.0f); setParam(apvts, ParamIDs::eqGain(0), -3.0f); }
        if (n == "Event Horizon Pad") { setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.4f); setParam(apvts, ParamIDs::glueTime, 0.7f); setParam(apvts, ParamIDs::eqOn(0), 1.0f); setParam(apvts, ParamIDs::eqGain(0), -4.0f); setParam(apvts, ParamIDs::eqOn(3), 1.0f); setParam(apvts, ParamIDs::eqGain(3), 2.0f); }
        if (n == "Ion Growl")         { setParam(apvts, ParamIDs::satOn, 1.0f); setParam(apvts, ParamIDs::satType, 2.0f); setParam(apvts, ParamIDs::satLow, 120.0f); setParam(apvts, ParamIDs::satDrive, 0.55f); setParam(apvts, ParamIDs::glueOn, 1.0f); setParam(apvts, ParamIDs::glueDepth, 0.6f); setParam(apvts, ParamIDs::glueTime, 0.3f); setParam(apvts, ParamIDs::eqOn(1), 1.0f); setParam(apvts, ParamIDs::eqFreq(1), 350.0f); setParam(apvts, ParamIDs::eqGain(1), -3.0f); }
        if (n == "Halo Pluck")        { setParam(apvts, ParamIDs::echoOn, 1.0f); setParam(apvts, ParamIDs::echoDiv, 7.0f); setParam(apvts, ParamIDs::echoFb, 0.45f); setParam(apvts, ParamIDs::echoMix, 0.3f); setParam(apvts, ParamIDs::echoTone, 0.4f); }
        if (n == "Halo Pluck")        { band(0, 2.0f, 0.3f, 1.8f); band(1, 4.0f, 0.3f, 1.5f); }
    }
}
