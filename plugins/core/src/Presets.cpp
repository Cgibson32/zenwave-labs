#include "Presets.h"
#include "PluginProcessor.h"
#include <zenwave_ui/Modulation.h>
#include <zenwave_ui/ProcessorHelpers.h>

namespace Presets
{
    // characters: 0 Tape, 1 Tube, 2 Fold, 3 Rectify, 4 Crush
    // name, category, drive, heat, focus, width, mix, outDb, lowX, highX, subLock, low{ch,drv,mix,gain}, mid{...}, high{...}
    static const Preset kPresets[] = {
        { "Init",                 "Init",    0.40f, 0.30f,  0.00f, 1.00f, 1.00f, 0.0f, 150.0f, 3500.0f, true,  { 0, 0.30f, 1.0f, 0.0f }, { 0, 0.30f, 1.0f, 0.0f }, { 0, 0.30f, 1.0f, 0.0f } },

        // --- Signature ---
        { "Growl Engine",         "Signature", 0.60f, 0.50f,  0.00f, 1.00f, 1.00f, 0.0f, 120.0f, 3000.0f, true,  { 0, 0.20f, 1.0f, 0.0f }, { 1, 0.60f, 1.0f, 0.5f }, { 2, 0.40f, 0.9f, 0.0f } },
        { "808 Glue",             "Signature", 0.40f, 0.40f, -0.30f, 1.00f, 1.00f, 0.0f, 90.0f,  2000.0f, false, { 1, 0.35f, 1.0f, 0.0f }, { 0, 0.30f, 1.0f, 0.0f }, { 0, 0.20f, 0.8f, 0.0f } },
        { "Vocal Presence",       "Signature", 0.35f, 0.55f, -0.40f, 1.00f, 0.60f, 0.0f, 200.0f, 4500.0f, true,  { 0, 0.10f, 1.0f, 0.0f }, { 1, 0.45f, 1.0f, 0.0f }, { 5, 0.30f, 1.0f, 0.5f } },
        { "Drum Bus Heat",        "Signature", 0.35f, 0.40f,  0.00f, 1.05f, 1.00f, 0.0f, 110.0f, 4000.0f, true,  { 0, 0.30f, 1.0f, 0.0f }, { 1, 0.40f, 1.0f, 0.0f }, { 0, 0.35f, 1.0f, 0.0f } },
        { "Neuro Talk",           "Signature", 0.80f, 0.80f,  0.00f, 1.00f, 1.00f, 0.0f, 150.0f, 4000.0f, true,  { 0, 0.15f, 1.0f, 0.0f }, { 2, 0.75f, 1.0f, 0.0f }, { 3, 0.70f, 1.0f, 1.0f } },

        // --- Bass ---
        { "Growl Glue",           "Bass",    0.55f, 0.45f,  0.00f, 1.00f, 1.00f, 0.0f, 120.0f, 2500.0f, true,  { 0, 0.20f, 1.0f, 0.0f }, { 1, 0.65f, 1.0f, 0.5f }, { 0, 0.45f, 0.9f, 0.0f } },
        { "Riddim Crunch",        "Bass",    0.70f, 0.60f,  0.00f, 1.00f, 1.00f, 0.0f, 100.0f, 1800.0f, true,  { 0, 0.10f, 1.0f, 0.0f }, { 3, 0.75f, 1.0f, 1.0f }, { 2, 0.50f, 0.8f, 0.0f } },
        { "Wonky Fuzz",           "Bass",    0.80f, 0.70f,  0.00f, 1.10f, 1.00f, 0.0f, 140.0f, 3000.0f, true,  { 0, 0.15f, 1.0f, 0.0f }, { 2, 0.85f, 1.0f, 0.0f }, { 1, 0.60f, 1.0f, 1.0f } },
        { "Sub-Safe Smash",       "Bass",    0.90f, 0.50f,  0.00f, 1.00f, 1.00f, 0.0f, 90.0f,  2200.0f, true,  { 0, 0.00f, 1.0f, 0.0f }, { 0, 0.95f, 1.0f, 0.0f }, { 0, 0.90f, 1.0f, 0.0f } },
        { "Neuro Bite",           "Bass",    0.75f, 0.80f,  0.00f, 1.00f, 1.00f, 0.0f, 160.0f, 4500.0f, true,  { 1, 0.25f, 1.0f, 0.0f }, { 2, 0.70f, 1.0f, 0.0f }, { 3, 0.80f, 1.0f, 2.0f } },
        { "Halftime Heat",        "Bass",    0.50f, 0.65f,  0.00f, 1.00f, 0.80f, 0.0f, 130.0f, 3200.0f, true,  { 1, 0.30f, 1.0f, 0.0f }, { 1, 0.55f, 1.0f, 0.0f }, { 0, 0.40f, 0.7f, -1.0f } },
        { "808 Tube Glue",        "Bass",    0.45f, 0.55f,  0.00f, 1.00f, 1.00f, 0.0f, 70.0f,  1200.0f, false, { 1, 0.35f, 0.8f, 0.0f }, { 1, 0.50f, 1.0f, 0.0f }, { 0, 0.30f, 0.6f, 0.0f } },

        // --- Vocals ---
        { "Vocal Warmth",         "Vocals",  0.35f, 0.50f,  0.00f, 1.00f, 1.00f, 0.0f, 180.0f, 4000.0f, true,  { 0, 0.20f, 1.0f, 0.0f }, { 1, 0.40f, 1.0f, 0.0f }, { 0, 0.25f, 0.8f, 0.5f } },
        { "Radio Vox",            "Vocals",  0.60f, 0.40f,  0.00f, 0.80f, 1.00f, 0.0f, 300.0f, 3000.0f, true,  { 0, 0.00f, 0.0f, -12.0f }, { 3, 0.70f, 1.0f, 2.0f }, { 0, 0.30f, 0.3f, -6.0f } },
        { "Grit Vox (Parallel)",  "Vocals",  0.85f, 0.70f, -0.60f, 1.00f, 0.35f, 0.0f, 200.0f, 3500.0f, true,  { 0, 0.30f, 1.0f, 0.0f }, { 2, 0.80f, 1.0f, 0.0f }, { 1, 0.60f, 1.0f, 0.0f } },

        // --- Drums ---
        { "Drum Bus Glue",        "Drums",   0.30f, 0.35f,  0.00f, 1.00f, 1.00f, 0.0f, 110.0f, 4000.0f, true,  { 0, 0.25f, 1.0f, 0.0f }, { 0, 0.35f, 1.0f, 0.0f }, { 0, 0.30f, 1.0f, 0.0f } },
        { "Snare Crack",          "Drums",   0.65f, 0.60f,  0.00f, 1.00f, 1.00f, 0.0f, 200.0f, 2500.0f, true,  { 0, 0.10f, 1.0f, 0.0f }, { 1, 0.60f, 1.0f, 1.0f }, { 2, 0.55f, 0.8f, 1.5f } },
        { "Kick Punch Tape",      "Drums",   0.50f, 0.30f,  0.00f, 1.00f, 1.00f, 0.0f, 80.0f,  1500.0f, false, { 0, 0.55f, 1.0f, 0.0f }, { 0, 0.45f, 1.0f, 0.0f }, { 0, 0.20f, 0.6f, 0.0f } },
        { "Hat Air Fuzz",         "Drums",   0.55f, 0.50f,  0.80f, 1.30f, 1.00f, 0.0f, 250.0f, 6000.0f, true,  { 0, 0.00f, 0.0f, 0.0f }, { 0, 0.20f, 0.5f, 0.0f }, { 2, 0.60f, 1.0f, 1.0f } },

        // --- Master / Bus ---
        { "Glue Tape",            "Master",  0.25f, 0.25f,  0.00f, 1.00f, 1.00f, 0.0f, 120.0f, 5000.0f, true,  { 0, 0.15f, 1.0f, 0.0f }, { 0, 0.25f, 1.0f, 0.0f }, { 0, 0.20f, 1.0f, 0.0f } },
        { "Loudness Heat",        "Master",  0.45f, 0.45f,  0.00f, 1.00f, 1.00f, 0.0f, 100.0f, 4500.0f, true,  { 0, 0.20f, 1.0f, 0.0f }, { 1, 0.45f, 1.0f, 0.0f }, { 1, 0.40f, 1.0f, 0.5f } },
        { "Wide Air",             "Master",  0.30f, 0.60f,  0.90f, 1.35f, 1.00f, 0.0f, 150.0f, 7000.0f, true,  { 0, 0.00f, 1.0f, 0.0f }, { 0, 0.15f, 1.0f, 0.0f }, { 1, 0.45f, 1.0f, 1.0f } },

        // --- FX ---
        { "Total Fold",           "FX",      1.00f, 0.80f,  0.00f, 1.00f, 1.00f, 0.0f, 150.0f, 3500.0f, true,  { 0, 0.20f, 1.0f, 0.0f }, { 2, 1.00f, 1.0f, 0.0f }, { 2, 1.00f, 1.0f, 0.0f } },
        { "Bit Storm",            "FX",      0.80f, 0.30f,  0.00f, 1.00f, 1.00f, 0.0f, 120.0f, 3000.0f, true,  { 0, 0.10f, 1.0f, 0.0f }, { 4, 0.75f, 1.0f, 0.0f }, { 4, 0.90f, 1.0f, 0.0f } },
        { "Broken Speaker",       "FX",      0.90f, 1.00f,  0.00f, 0.70f, 1.00f, 0.0f, 350.0f, 2500.0f, true,  { 0, 0.00f, 0.0f, -18.0f }, { 3, 0.95f, 1.0f, 3.0f }, { 2, 0.70f, 0.5f, -6.0f } },
        { "Rectifier",            "FX",      0.70f, 0.90f,  0.00f, 1.00f, 1.00f, 0.0f, 150.0f, 3500.0f, true,  { 3, 0.40f, 1.0f, 0.0f }, { 3, 0.80f, 1.0f, 0.0f }, { 3, 0.80f, 1.0f, 0.0f } },
    };

    int count() { return int(std::size(kPresets)); }
    const Preset& get(int index) { return kPresets[juce::jlimit(0, count() - 1, index)]; }
    juce::String name(int index) { return get(index).name; }
    juce::String category(int index) { return get(index).category; }

    static void setParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float plainValue)
    {
        if (auto* p = apvts.getParameter(id))
        {
            const float norm = p->convertTo0to1(plainValue);
            p->beginChangeGesture(); p->setValueNotifyingHost(norm); p->endChangeGesture();
        }
    }

    void apply(int index, juce::AudioProcessorValueTreeState& apvts)
    {
        const auto& p = get(index);
        setParam(apvts, ParamIDs::drive, p.drive);   setParam(apvts, ParamIDs::heat, p.heat);     setParam(apvts, ParamIDs::focus, p.focus);
        setParam(apvts, ParamIDs::width, p.width);   setParam(apvts, ParamIDs::mix, p.mix);       setParam(apvts, ParamIDs::output, p.outputDb);
        setParam(apvts, ParamIDs::lowX, p.lowX);     setParam(apvts, ParamIDs::highX, p.highX);   setParam(apvts, ParamIDs::subLock, p.subLock ? 1.0f : 0.0f);
        const Band* bands[3] = { &p.low, &p.mid, &p.high };
        for (int b = 0; b < 3; ++b)
        {
            setParam(apvts, ParamIDs::bandChar(b), float(bands[b]->character));
            setParam(apvts, ParamIDs::bandDrive(b), bands[b]->drive);
            setParam(apvts, ParamIDs::bandMix(b), bands[b]->mix);
            setParam(apvts, ParamIDs::bandGain(b), bands[b]->gainDb);
        }
        // newer params: deterministic defaults, then per-preset character
        const float tuneDef[3] = { 80.0f, 800.0f, 6000.0f };
        for (int b = 0; b < 3; ++b) { setParam(apvts, ParamIDs::bandOn(b), 1.0f); setParam(apvts, ParamIDs::bandSolo(b), 0.0f); setParam(apvts, ParamIDs::bandTune(b), tuneDef[b]); setParam(apvts, ParamIDs::bandRes(b), 0.0f); }
        setParam(apvts, ParamIDs::vowel, 0.0f); setParam(apvts, ParamIDs::punch, 0.0f);
        setParam(apvts, ParamIDs::crushOn, 0.0f); setParam(apvts, ParamIDs::crushBits, 12.0f); setParam(apvts, ParamIDs::crushRate, 0.0f); setParam(apvts, ParamIDs::crushMix, 1.0f);
        {
            const juce::String nm(p.name);
            // Signature set
            if (nm == "Growl Engine")   { setParam(apvts, ParamIDs::vowel, 1.0f); setParam(apvts, ParamIDs::bandTune(1), 500.0f); setParam(apvts, ParamIDs::bandRes(1), 0.55f); setParam(apvts, ParamIDs::punch, -0.3f); setParam(apvts, ParamIDs::highCut, 14000.0f); }
            if (nm == "808 Glue")       { setParam(apvts, ParamIDs::punch, -0.5f); }
            if (nm == "Vocal Presence") { setParam(apvts, ParamIDs::bandTune(1), 2200.0f); setParam(apvts, ParamIDs::bandRes(1), 0.30f); setParam(apvts, ParamIDs::punch, -0.3f); setParam(apvts, ParamIDs::highCut, 16000.0f); }
            if (nm == "Drum Bus Heat")  { setParam(apvts, ParamIDs::punch, -0.6f); }
            if (nm == "Neuro Talk")     { setParam(apvts, ParamIDs::vowel, 1.0f); setParam(apvts, ParamIDs::bandTune(1), 800.0f); setParam(apvts, ParamIDs::bandRes(1), 0.60f); setParam(apvts, ParamIDs::bandRes(2), 0.40f); setParam(apvts, ParamIDs::bandTune(2), 5000.0f);
                                          setParam(apvts, ParamIDs::crushOn, 1.0f); setParam(apvts, ParamIDs::crushBits, 9.0f); setParam(apvts, ParamIDs::crushRate, 0.2f); setParam(apvts, ParamIDs::crushMix, 0.35f); }
            if (nm == "Neuro Bite")     { setParam(apvts, ParamIDs::bandTune(1), 900.0f);  setParam(apvts, ParamIDs::bandRes(1), 0.55f); setParam(apvts, ParamIDs::bandTune(2), 5200.0f); setParam(apvts, ParamIDs::bandRes(2), 0.35f); }
            if (nm == "Riddim Crunch")  { setParam(apvts, ParamIDs::bandTune(1), 650.0f);  setParam(apvts, ParamIDs::bandRes(1), 0.45f); }
            if (nm == "Wonky Fuzz")     { setParam(apvts, ParamIDs::bandTune(1), 1200.0f); setParam(apvts, ParamIDs::bandRes(1), 0.60f); }
            if (nm == "Broken Speaker") { setParam(apvts, ParamIDs::bandTune(1), 1800.0f); setParam(apvts, ParamIDs::bandRes(1), 0.75f); setParam(apvts, ParamIDs::crushOn, 1.0f); setParam(apvts, ParamIDs::crushBits, 9.0f); setParam(apvts, ParamIDs::crushRate, 0.25f); setParam(apvts, ParamIDs::crushMix, 0.6f); }
            if (nm == "Bit Storm")      { setParam(apvts, ParamIDs::crushOn, 1.0f); setParam(apvts, ParamIDs::crushBits, 6.0f); setParam(apvts, ParamIDs::crushRate, 0.5f); setParam(apvts, ParamIDs::crushMix, 1.0f); }
            if (nm == "Radio Vox")      { setParam(apvts, ParamIDs::bandTune(1), 2200.0f); setParam(apvts, ParamIDs::bandRes(1), 0.50f); setParam(apvts, ParamIDs::bandOn(0), 0.0f); }
            if (nm == "Growl Glue")     { setParam(apvts, ParamIDs::vowel, 1.0f); setParam(apvts, ParamIDs::bandTune(1), 400.0f); setParam(apvts, ParamIDs::bandRes(1), 0.5f); }   // talking growl
            if (nm == "Drum Bus Glue")  { setParam(apvts, ParamIDs::punch, -0.6f); }   // hits stay clean, body saturates
            if (nm == "Kick Punch Tape"){ setParam(apvts, ParamIDs::punch, -0.8f); }
            if (nm == "Vocal Warmth")   { setParam(apvts, ParamIDs::punch, -0.4f); }
            if (nm == "Snare Crack")    { setParam(apvts, ParamIDs::punch, 0.5f); setParam(apvts, ParamIDs::bandTune(1), 220.0f);  setParam(apvts, ParamIDs::bandRes(1), 0.40f); }
        }
        setParam(apvts, ParamIDs::power, 1.0f);
        setParam(apvts, ParamIDs::lowCut, 20.0f); setParam(apvts, ParamIDs::highCut, 20000.0f);
        setParam(apvts, ParamIDs::lowSlope, 1.0f); setParam(apvts, ParamIDs::highSlope, 1.0f); setParam(apvts, ParamIDs::tilt, 0.0f);

        // Factory motion (targets 1-based: Drive=1 Heat Focus Width Mix LowX HighX LowDrive MidDrive HighDrive LowCut HighCut Tilt=13
        //                                   LowTune MidTune HighTune LowRes MidRes HighRes CrushBits CrushRate Punch=22)
        const juce::String n(p.name);
        if (n == "Growl Glue")     { zwui::setLfo(apvts, 0, 7, true, 10, 1.0f); zwui::setModSlot(apvts, 1, 1, 15, 0.45f); }   // step lane walks the vowel
        if (n == "Growl Engine")   { zwui::setLfo(apvts, 0, 7, true, 10, 1.0f); zwui::setModSlot(apvts, 0, 1, 15, 0.45f);
                                     zwui::setEnv(apvts, 3.0f, 200.0f, 0.2f, 250.0f); zwui::setModSlot(apvts, 1, 3, 9, 0.30f); }     // vowel walks the bar; hits bite harder
        if (n == "Vocal Presence") { zwui::setEnv(apvts, 5.0f, 150.0f, 1.0f, 400.0f); zwui::setModSlot(apvts, 0, 3, 1, -0.20f); }   // loud phrases ease the drive back
        if (n == "Drum Bus Heat")  { zwui::setEnv(apvts, 1.0f, 80.0f, 0.0f, 120.0f);  zwui::setModSlot(apvts, 0, 3, 2, 0.20f); }    // transients get a touch more heat
        if (n == "Neuro Talk")     { zwui::setLfo(apvts, 0, 7, true, 9, 1.0f);  zwui::setModSlot(apvts, 0, 1, 15, 0.50f);
                                     zwui::setLfo(apvts, 1, 5, true, 2, 1.0f);  zwui::setModSlot(apvts, 1, 2, 20, 0.30f);
                                     zwui::setEnv(apvts, 2.0f, 120.0f, 0.0f, 200.0f); zwui::setModSlot(apvts, 2, 3, 18, 0.25f); }  // steps talk, S&H stutters the bits, hits sharpen the vowel
        if (n == "Growl Glue")     { zwui::setModSlot(apvts, 0, 3, 9, 0.35f); }                                   // env -> mid drive: growls bite harder on hits
        if (n == "Neuro Bite")     { zwui::setLfo(apvts, 0, 5, true, 2, 1.0f); zwui::setModSlot(apvts, 0, 1, 7, 0.25f); } // S&H 1/16 on the high crossover
        if (n == "Wonky Fuzz")     { zwui::setLfo(apvts, 0, 6, true, 8, 1.0f); zwui::setModSlot(apvts, 0, 1, 2, 0.30f); zwui::setModSlot(apvts, 1, 1, 15, 0.20f); }   // drift LFO also wanders the mid resonance
        if (n == "Bit Storm")      { zwui::setLfo(apvts, 0, 5, true, 2, 1.0f); zwui::setModSlot(apvts, 0, 1, 20, 0.35f); }   // S&H 1/16 on crush bits
        if (n == "Hat Air Fuzz")   { zwui::setModSlot(apvts, 0, 3, 1, -0.30f); }                                  // env -> drive down on hits = keeps transients
    }
}
