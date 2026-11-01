#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
using namespace nebula;

NebulaAudioProcessor::NebulaAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    for (int o = 0; o < 2; ++o)
    {
        pTable[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "table"));   pPos[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "position"));
        pLevel[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "level"));   pOct[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "octave"));
        pSemi[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "semis"));    pFine[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "fine"));
        pUni[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "unison"));    pSpread[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "spread"));
        pWidth[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "width"));   pOn[o] = apvts.getRawParameterValue(ParamIDs::osc(o, "on"));
    }
    for (int i = 0; i < kCarveBands; ++i) { pCOn[i] = apvts.getRawParameterValue(ParamIDs::carveBandOn(i)); pCCentre[i] = apvts.getRawParameterValue(ParamIDs::carveBandCentre(i)); pCWidth[i] = apvts.getRawParameterValue(ParamIDs::carveBandWidth(i)); pCGain[i] = apvts.getRawParameterValue(ParamIDs::carveBandGain(i)); }
    pCarveOn = apvts.getRawParameterValue(ParamIDs::carveOn);
    pCutoff = apvts.getRawParameterValue(ParamIDs::cutoff); pRes = apvts.getRawParameterValue(ParamIDs::res); pFilterMode = apvts.getRawParameterValue(ParamIDs::filterMode);
    pEnvAmt = apvts.getRawParameterValue(ParamIDs::envAmt); pKeyTrack = apvts.getRawParameterValue(ParamIDs::keyTrack); pDrive = apvts.getRawParameterValue(ParamIDs::drive);
    pDrift = apvts.getRawParameterValue(ParamIDs::drift); pGlide = apvts.getRawParameterValue(ParamIDs::glide);
    const char* ampIds[4] = { ParamIDs::ampA, ParamIDs::ampD, ParamIDs::ampS, ParamIDs::ampR }; const char* modIds[4] = { ParamIDs::modA, ParamIDs::modD, ParamIDs::modS, ParamIDs::modR };
    for (int i = 0; i < 4; ++i) { pAmp[i] = apvts.getRawParameterValue(ampIds[i]); pModEnv[i] = apvts.getRawParameterValue(modIds[i]); }
    pSpaceMix = apvts.getRawParameterValue(ParamIDs::spaceMix); pSpaceSize = apvts.getRawParameterValue(ParamIDs::spaceSize); pSpaceDecay = apvts.getRawParameterValue(ParamIDs::spaceDecay); pSpaceDark = apvts.getRawParameterValue(ParamIDs::spaceDark);
    const char* macroIds[8] = { ParamIDs::mWarmth, ParamIDs::mAir, ParamIDs::mMovement, ParamIDs::mSpace, ParamIDs::mGrit, ParamIDs::mWidth, ParamIDs::mAttack, ParamIDs::mRelease };
    for (int i = 0; i < 8; ++i) pMacro[i] = apvts.getRawParameterValue(macroIds[i]);
    pVolume = apvts.getRawParameterValue(ParamIDs::volume); pPower = apvts.getRawParameterValue(ParamIDs::power);
    pDuck = apvts.getRawParameterValue(ParamIDs::duck); pDuckRelease = apvts.getRawParameterValue(ParamIDs::duckRelease);
    pSatType = apvts.getRawParameterValue(ParamIDs::satType); pSatBias = apvts.getRawParameterValue(ParamIDs::satBias); pSatLow = apvts.getRawParameterValue(ParamIDs::satLow); pSatOut = apvts.getRawParameterValue(ParamIDs::satOut);
    pGlueUp = apvts.getRawParameterValue(ParamIDs::glueUp); pGlueDown = apvts.getRawParameterValue(ParamIDs::glueDown); pGlueMix = apvts.getRawParameterValue(ParamIDs::glueMix); pGlueOut = apvts.getRawParameterValue(ParamIDs::glueOut);
    pEchoWidth = apvts.getRawParameterValue(ParamIDs::echoWidth);
    pChorusOn = apvts.getRawParameterValue(ParamIDs::chorusOn); pChorusMode = apvts.getRawParameterValue(ParamIDs::chorusMode); pChorusRate = apvts.getRawParameterValue(ParamIDs::chorusRate);
    pSpaceOn = apvts.getRawParameterValue(ParamIDs::spaceOn); pSpacePre = apvts.getRawParameterValue(ParamIDs::spacePre); pSpaceLow = apvts.getRawParameterValue(ParamIDs::spaceLow); pSpaceShimmer = apvts.getRawParameterValue(ParamIDs::spaceShimmer);
    for (int i = 0; i < 4; ++i) { pEqOn[i] = apvts.getRawParameterValue(ParamIDs::eqOn(i)); pEqType[i] = apvts.getRawParameterValue(ParamIDs::eqType(i)); pEqFreq[i] = apvts.getRawParameterValue(ParamIDs::eqFreq(i)); pEqQ[i] = apvts.getRawParameterValue(ParamIDs::eqQ(i)); }
    pSatOn = apvts.getRawParameterValue(ParamIDs::satOn); pSatMix = apvts.getRawParameterValue(ParamIDs::satMix);
    pGlueOn = apvts.getRawParameterValue(ParamIDs::glueOn); pGlueTime = apvts.getRawParameterValue(ParamIDs::glueTime);
    pEchoOn = apvts.getRawParameterValue(ParamIDs::echoOn); pEchoSync = apvts.getRawParameterValue(ParamIDs::echoSync); pEchoDiv = apvts.getRawParameterValue(ParamIDs::echoDiv);
    pEchoTime = apvts.getRawParameterValue(ParamIDs::echoTime); pEchoPP = apvts.getRawParameterValue(ParamIDs::echoPP);

    mod.attach(apvts, nebulaModTargets());
    bank.setAlwaysLocked({ ParamIDs::power, ParamIDs::volume, ParamIDs::duck, ParamIDs::duckRelease, ParamIDs::glide });
    for (int i = 0; i < kCarveBands; ++i) bank.setAlwaysLocked({ ParamIDs::carveBandOn(i).toRawUTF8(), ParamIDs::carveBandCentre(i).toRawUTF8(), ParamIDs::carveBandWidth(i).toRawUTF8(), ParamIDs::carveBandGain(i).toRawUTF8() });
    bank.setSafeRange(ParamIDs::cutoff, 200.0f, 16000.0f); bank.setSafeRange(ParamIDs::res, 0.0f, 0.7f);
    bank.setSafeRange("aoctave", -1.0f, 1.0f); bank.setSafeRange("boctave", -1.0f, 1.0f); bank.setSafeRange("asemis", 0.0f, 0.0f); bank.setSafeRange("bsemis", -7.0f, 7.0f);
    bank.setSafeRange(ParamIDs::spaceMix, 0.0f, 0.6f);

    // listen for anything that changes the rendered tables
    struct L : juce::AudioProcessorValueTreeState::Listener { NebulaAudioProcessor& p; L(NebulaAudioProcessor& pp) : p(pp) {} void parameterChanged(const juce::String&, float) override { p.markTablesDirty(); } };
    tableListener = std::make_unique<L>(*this);
    for (int o = 0; o < 2; ++o) apvts.addParameterListener(ParamIDs::osc(o, "table"), tableListener.get());
    for (int i = 0; i < kCarveBands; ++i) for (auto id : { ParamIDs::carveBandOn(i), ParamIDs::carveBandCentre(i), ParamIDs::carveBandWidth(i), ParamIDs::carveBandGain(i) }) apvts.addParameterListener(id, tableListener.get());
    apvts.addParameterListener(ParamIDs::carveOn, tableListener.get());
    rebuildTablesIfNeeded();
    startTimerHz(20);
}

NebulaAudioProcessor::~NebulaAudioProcessor()
{
    stopTimer();
    for (int o = 0; o < 2; ++o) apvts.removeParameterListener(ParamIDs::osc(o, "table"), tableListener.get());
    for (int i = 0; i < kCarveBands; ++i) for (auto id : { ParamIDs::carveBandOn(i), ParamIDs::carveBandCentre(i), ParamIDs::carveBandWidth(i), ParamIDs::carveBandGain(i) }) apvts.removeParameterListener(id, tableListener.get());
    apvts.removeParameterListener(ParamIDs::carveOn, tableListener.get());
}

APVTS::ParameterLayout NebulaAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat; using B = juce::AudioParameterBool; using C = juce::AudioParameterChoice; using I = juce::AudioParameterInt; using Rng = juce::NormalisableRange<float>;
    auto pctAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; });
    auto msAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return v < 1000.0f ? juce::String(juce::roundToInt(v)) + " ms" : juce::String(v / 1000.0f, 2) + " s"; });
    auto hzAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; });
    auto pct = [&](const juce::String& id, const juce::String& name, float def) { return std::make_unique<P>(juce::ParameterID{ id, 1 }, name, Rng{ 0.0f, 1.0f, 0.001f }, def, pctAttr); };
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (int o = 0; o < 2; ++o)
    {
        const juce::String n = o == 0 ? "A " : "B ";
        params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::osc(o, "table"), 1 }, n + "Table", tableNames(), o == 0 ? 0 : 5));
        params.push_back(pct(ParamIDs::osc(o, "position"), n + "Position", o == 0 ? 0.3f : 0.5f));
        params.push_back(pct(ParamIDs::osc(o, "level"), n + "Level", o == 0 ? 0.8f : 0.5f));
        params.push_back(std::make_unique<I>(juce::ParameterID{ ParamIDs::osc(o, "octave"), 1 }, n + "Octave", -3, 3, 0));
        params.push_back(std::make_unique<I>(juce::ParameterID{ ParamIDs::osc(o, "semis"), 1 }, n + "Semitones", -12, 12, 0));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::osc(o, "fine"), 1 }, n + "Fine", Rng{ -100.0f, 100.0f, 0.1f }, o == 0 ? 0.0f : 6.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v)) + " ct"; })));
        params.push_back(std::make_unique<I>(juce::ParameterID{ ParamIDs::osc(o, "unison"), 1 }, n + "Unison", 1, 8, o == 0 ? 3 : 2));
        params.push_back(pct(ParamIDs::osc(o, "spread"), n + "Spread", 0.25f));
        params.push_back(pct(ParamIDs::osc(o, "width"), n + "Width", 0.7f));
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::osc(o, "on"), 1 }, n + "On", true));
    }
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::carveOn, 1 }, "Carve", true));
    const float carveDef[kCarveBands] = { 2.0f, 3.0f, 5.0f, 8.0f, 13.0f, 21.0f, 34.0f, 55.0f };
    for (int i = 0; i < kCarveBands; ++i)
    {
        const juce::String n = "Carve " + juce::String(i + 1) + " ";
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::carveBandOn(i), 1 }, n + "On", false));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::carveBandCentre(i), 1 }, n + "Centre", Rng{ 1.0f, 256.0f, 0.01f, 0.25f }, carveDef[i],
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return "H" + juce::String(v, v < 10 ? 1 : 0); })));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::carveBandWidth(i), 1 }, n + "Width", Rng{ 0.05f, 2.0f, 0.001f, 0.5f }, 0.4f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 2) + " oct"; })));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::carveBandGain(i), 1 }, n + "Gain", Rng{ 0.0f, 4.0f, 0.001f, 0.5f }, 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return v < 0.002f ? juce::String("cut") : juce::String(juce::Decibels::gainToDecibels(v), 1) + " dB"; })));
    }
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::cutoff, 1 }, "Cutoff", Rng{ 20.0f, 20000.0f, 0.1f, 0.25f }, 9000.0f, hzAttr));
    params.push_back(pct(ParamIDs::res, "Resonance", 0.2f));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::filterMode, 1 }, "Filter", juce::StringArray{ "Low", "High", "Band" }, 0));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::envAmt, 1 }, "Env Amount", Rng{ -1.0f, 1.0f, 0.001f }, 0.2f, pctAttr));
    params.push_back(pct(ParamIDs::keyTrack, "Key Track", 0.5f));
    params.push_back(pct(ParamIDs::drive, "Drive", 0.0f));
    params.push_back(pct(ParamIDs::drift, "Drift", 0.25f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::glide, 1 }, "Glide", Rng{ 0.0f, 2000.0f, 1.0f, 0.4f }, 0.0f, msAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::ampA, 1 }, "Amp Attack", Rng{ 0.5f, 5000.0f, 0.1f, 0.3f }, 20.0f, msAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::ampD, 1 }, "Amp Decay", Rng{ 1.0f, 5000.0f, 0.1f, 0.3f }, 300.0f, msAttr));
    params.push_back(pct(ParamIDs::ampS, "Amp Sustain", 0.8f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::ampR, 1 }, "Amp Release", Rng{ 5.0f, 10000.0f, 0.1f, 0.3f }, 600.0f, msAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::modA, 1 }, "Mod Attack", Rng{ 0.5f, 5000.0f, 0.1f, 0.3f }, 10.0f, msAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::modD, 1 }, "Mod Decay", Rng{ 1.0f, 5000.0f, 0.1f, 0.3f }, 400.0f, msAttr));
    params.push_back(pct(ParamIDs::modS, "Mod Sustain", 0.3f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::modR, 1 }, "Mod Release", Rng{ 5.0f, 10000.0f, 0.1f, 0.3f }, 400.0f, msAttr));
    params.push_back(pct(ParamIDs::spaceMix, "Space Mix", 0.25f));
    params.push_back(pct(ParamIDs::spaceSize, "Space Size", 0.7f));
    params.push_back(pct(ParamIDs::spaceDecay, "Space Decay", 0.6f));
    params.push_back(pct(ParamIDs::spaceDark, "Space Dark", 0.5f));
    params.push_back(pct(ParamIDs::mWarmth, "Warmth", 0.3f));   params.push_back(pct(ParamIDs::mAir, "Air", 0.3f));
    params.push_back(pct(ParamIDs::mMovement, "Movement", 0.2f)); params.push_back(pct(ParamIDs::mSpace, "Space", 0.4f));
    params.push_back(pct(ParamIDs::mGrit, "Grit", 0.0f));       params.push_back(pct(ParamIDs::mWidth, "Width", 0.6f));
    params.push_back(pct(ParamIDs::mAttack, "Attack", 0.5f));   params.push_back(pct(ParamIDs::mRelease, "Release", 0.5f));
    // FX page
    const float eqDef[4] = { 120.0f, 500.0f, 2500.0f, 9000.0f }; const int eqTypeDef[4] = { 1, 0, 0, 2 };
    for (int i = 0; i < 4; ++i)
    {
        const juce::String n = "EQ " + juce::String(i + 1) + " ";
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::eqOn(i), 1 }, n + "On", false));
        params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::eqType(i), 1 }, n + "Type", juce::StringArray{ "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut" }, eqTypeDef[i]));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::eqFreq(i), 1 }, n + "Freq", Rng{ 20.0f, 20000.0f, 0.1f, 0.25f }, eqDef[i], hzAttr));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::eqGain(i), 1 }, n + "Gain", Rng{ -18.0f, 18.0f, 0.1f }, 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB"; })));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::eqQ(i), 1 }, n + "Q", Rng{ 0.2f, 12.0f, 0.01f, 0.4f }, 1.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return "Q " + juce::String(v, 1); })));
    }
    auto dbAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB"; });
    auto biAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { const int p = juce::roundToInt(v * 100.0f); return (p > 0 ? "+" : "") + juce::String(p) + " %"; });
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::satOn, 1 }, "Saturate", false));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::satType, 1 }, "Sat Type", juce::StringArray{ "Tape", "Tube", "Fold", "Rectify", "Crush", "Diode", "Octave", "Hard" }, 0));
    params.push_back(pct(ParamIDs::satDrive, "Sat Drive", 0.3f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::satTone, 1 }, "Sat Tone", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f, biAttr));
    params.push_back(pct(ParamIDs::satBias, "Sat Bias", 0.0f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::satLow, 1 }, "Sat Keep Lows", Rng{ 20.0f, 500.0f, 0.1f, 0.4f }, 20.0f, hzAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::satOut, 1 }, "Sat Out", Rng{ -12.0f, 12.0f, 0.1f }, 0.0f, dbAttr));
    params.push_back(pct(ParamIDs::satMix, "Sat Mix", 1.0f));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::glueOn, 1 }, "Glue", false));
    params.push_back(pct(ParamIDs::glueDepth, "Glue Depth", 0.5f)); params.push_back(pct(ParamIDs::glueTime, "Glue Time", 0.5f));
    params.push_back(pct(ParamIDs::glueUp, "Glue Up", 1.0f)); params.push_back(pct(ParamIDs::glueDown, "Glue Down", 1.0f)); params.push_back(pct(ParamIDs::glueMix, "Glue Mix", 1.0f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::glueOut, 1 }, "Glue Out", Rng{ -12.0f, 12.0f, 0.1f }, 0.0f, dbAttr));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::echoOn, 1 }, "Echo", false));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::echoSync, 1 }, "Echo Sync", true));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::echoDiv, 1 }, "Echo Division", zwui::divisionNames(), 7));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::echoTime, 1 }, "Echo Time", Rng{ 20.0f, 2000.0f, 0.1f, 0.4f }, 375.0f, msAttr));
    params.push_back(pct(ParamIDs::echoFb, "Echo Feedback", 0.4f)); params.push_back(pct(ParamIDs::echoMix, "Echo Mix", 0.25f)); params.push_back(pct(ParamIDs::echoTone, "Echo Tone", 0.5f));
    params.push_back(pct(ParamIDs::echoWobble, "Echo Wobble", 0.0f)); params.push_back(pct(ParamIDs::echoWidth, "Echo Width", 0.66f));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::echoPP, 1 }, "Echo Ping-Pong", true));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::spaceOn, 1 }, "Space On", true));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::spacePre, 1 }, "Space Pre-Delay", Rng{ 0.0f, 250.0f, 0.1f, 0.5f }, 0.0f, msAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::spaceLow, 1 }, "Space Low Cut", Rng{ 20.0f, 800.0f, 0.1f, 0.4f }, 90.0f, hzAttr));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::spaceShimmer, 1 }, "Space Shimmer", false));
    params.push_back(pct(ParamIDs::vibrato, "Vibrato", 0.0f)); params.push_back(pct(ParamIDs::breath, "Breath", 0.0f)); params.push_back(pct(ParamIDs::evolve, "Evolve", 0.0f));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::chorusOn, 1 }, "Chorus", false));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::chorusMode, 1 }, "Chorus Mode", juce::StringArray{ "Chorus I", "Chorus II", "Ensemble" }, 2));
    params.push_back(pct(ParamIDs::chorusRate, "Chorus Rate", 0.4f)); params.push_back(pct(ParamIDs::chorusDepth, "Chorus Depth", 0.5f)); params.push_back(pct(ParamIDs::chorusMix, "Chorus Mix", 0.5f));
    params.push_back(pct(ParamIDs::duck, "Duck", 0.0f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::duckRelease, 1 }, "Duck Release", Rng{ 20.0f, 1500.0f, 1.0f, 0.5f }, 250.0f, msAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::volume, 1 }, "Volume", Rng{ -36.0f, 6.0f, 0.1f }, -6.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " dB"; })));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::power, 1 }, "Power", true));
    zwui::addModParams(params, nebulaModTargets());
    return { params.begin(), params.end() };
}

std::array<CarveBand, kCarveBands> NebulaAudioProcessor::carveBands() const
{
    std::array<CarveBand, kCarveBands> c;
    for (int i = 0; i < kCarveBands; ++i) { c[size_t(i)].on = pCOn[i]->load() > 0.5f; c[size_t(i)].centre = pCCentre[i]->load(); c[size_t(i)].widthOct = pCWidth[i]->load(); c[size_t(i)].gain = pCGain[i]->load(); }
    return c;
}


void NebulaAudioProcessor::rebuildTablesIfNeeded()
{
    // publish last pending (audio thread picks it up at block start)
    if (! tablesDirty.exchange(false)) return;
    const auto carve = carveBands(); const bool on = pCarveOn->load() > 0.5f;
    for (int o = 0; o < 2; ++o)
    {
        const int t = int(pTable[o]->load());
        if (t == renderedTable[o] && carve == renderedCarve && on == renderedCarveOn && uiTables[o]) continue;
        uiTables[o] = renderer.render(t, carve, on);
        renderedTable[o] = t;
    }
    renderedCarve = carve; renderedCarveOn = on;
    { const juce::SpinLock::ScopedLockType sl(tableLock); pendingTables[0] = uiTables[0]; pendingTables[1] = uiTables[1]; pendingReady.store(true); }
}

void NebulaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    for (auto& v : voices) { v.prepare(sampleRate); v.setParams(vp); v.setTables(audioTables); }
    reverb.prepare(sampleRate);
    fx.prepare(sampleRate);
    for (auto* s : { &smSpaceMix, &smVolume, &smPower }) { s->setSampleRate(sampleRate); s->setTime(30.0f); }
    smSpaceMix.reset(pSpaceMix->load()); smVolume.reset(zw::dbToGain(pVolume->load())); smPower.reset(pPower->load() > 0.5f ? 1.0f : 0.0f);
    for (auto& f : scLP) { f.setSampleRate(sampleRate); f.setLowpass(200.0f); }
    for (int c = 0; c < 2; ++c) { spacePreDl[c].prepare(sampleRate, 0.3f); shimmer[c].prepare(sampleRate); shimLP[c].setSampleRate(sampleRate); shimLP[c].setLowpass(5500.0f); shimHP[c].setSampleRate(sampleRate); shimHP[c].setHighpass(180.0f); }
    for (auto* sm : { &smSpacePre, &smSpaceOn, &smShimmer }) { sm->setSampleRate(sampleRate); sm->setTime(40.0f); }
    smSpacePre.setTime(120.0f); smSpacePre.reset(pSpacePre->load() * 0.001f * float(sampleRate)); smSpaceOn.reset(pSpaceOn->load() > 0.5f ? 1.0f : 0.0f); smShimmer.reset(0.0f); lastWetL = lastWetR = 0.0f;
    mod.prepare(sampleRate);
    meterOut.prepare(sampleRate);
    wet.setSize(2, samplesPerBlock);
    updateVoiceParams();
}

bool NebulaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    const auto& sc = layouts.getChannelSet(true, 0);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void NebulaAudioProcessor::updateVoiceParams()
{
    const auto M = [this](int t) { return mod.value(t); };
    const float warmth = M(ModT::Warmth), air = M(ModT::Air), movement = M(ModT::Movement), grit = M(ModT::Grit), width = M(ModT::Width);
    const float atkM = std::pow(10.0f, (pMacro[6]->load() - 0.5f) * 2.0f), relM = std::pow(10.0f, (pMacro[7]->load() - 0.5f) * 2.0f);   // 0.1x .. 10x
    for (int o = 0; o < 2; ++o)
    {
        auto& os = vp.osc[o];
        os.table = int(pTable[o]->load()); os.position = o == 0 ? M(ModT::PosA) : M(ModT::PosB);
        os.level = (o == 0 ? M(ModT::LevelA) : M(ModT::LevelB)) * (o == 1 ? 0.7f + 0.6f * air : 1.0f);
        os.octave = int(pOct[o]->load()); os.semis = int(pSemi[o]->load()); os.fine = pFine[o]->load();
        os.unison = int(pUni[o]->load()); os.spread = pSpread[o]->load() * (0.5f + width); os.width = pWidth[o]->load() * (0.4f + 1.0f * width);
        os.on = pOn[o]->load() > 0.5f;
    }
    vp.drift = M(ModT::Drift) * (0.5f + movement);
    vp.lfoToPosition = movement;
    vp.filterHz = M(ModT::Cutoff) * std::pow(2.0f, -2.5f * warmth) * std::pow(2.0f, 1.0f * air);
    vp.res = M(ModT::Res); vp.filterMode = int(pFilterMode->load()); vp.envAmt = pEnvAmt->load(); vp.keyTrack = pKeyTrack->load();
    vp.drive = juce::jlimit(0.0f, 1.0f, M(ModT::Drive) + grit * 0.8f + warmth * 0.15f);
    vp.ampA = pAmp[0]->load() * atkM; vp.ampD = pAmp[1]->load(); vp.ampS = pAmp[2]->load(); vp.ampR = pAmp[3]->load() * relM;
    vp.modA = pModEnv[0]->load(); vp.modD = pModEnv[1]->load(); vp.modS = pModEnv[2]->load(); vp.modR = pModEnv[3]->load();
    vp.glideMs = pGlide->load();
    vp.vibrato = juce::jlimit(0.0f, 1.0f, M(ModT::Vibrato)); vp.breath = juce::jlimit(0.0f, 1.0f, M(ModT::Breath) + air * 0.25f);
    vp.evolve = juce::jlimit(0.0f, 1.0f, M(ModT::Evolve) + movement * 0.4f);
    for (auto& v : voices) v.setParams(vp);
    reverb.setSize(juce::jlimit(0.0f, 1.0f, M(ModT::SpaceSize))); reverb.setDecay(juce::jlimit(0.0f, 1.0f, M(ModT::SpaceDecay)));
    reverb.setDamping(zw::lerp(12000.0f, 2500.0f, juce::jlimit(0.0f, 1.0f, M(ModT::SpaceDark))) * (0.7f + 0.6f * air)); reverb.setDiffusion(0.7f); reverb.setModDepth(0.3f + 0.4f * movement);
    reverb.setLoopHighpass(pSpaceLow->load());
    spaceWidthCur = 0.4f + juce::jlimit(0.0f, 1.0f, width) * 0.9f;     // reverb stereo width rides the WIDTH macro
    smSpacePre.setTarget(pSpacePre->load() * 0.001f * float(currentSampleRate)); smSpaceOn.setTarget(pSpaceOn->load() > 0.5f ? 1.0f : 0.0f); smShimmer.setTarget(pSpaceShimmer->load() > 0.5f ? 1.0f : 0.0f);
    smSpaceMix.setTarget(juce::jlimit(0.0f, 1.0f, M(ModT::SpaceMix) * (0.3f + 1.4f * pMacro[3]->load())));
    // FX chain
    const int eqG[4] = { ModT::Eq1Gain, ModT::Eq2Gain, ModT::Eq3Gain, ModT::Eq4Gain };
    for (int i = 0; i < 4; ++i) { fxp.eq[i].on = pEqOn[i]->load() > 0.5f; fxp.eq[i].type = int(pEqType[i]->load()); fxp.eq[i].freq = pEqFreq[i]->load(); fxp.eq[i].gainDb = M(eqG[i]); fxp.eq[i].q = pEqQ[i]->load(); }
    fxp.satOn = pSatOn->load() > 0.5f; fxp.satType = int(pSatType->load()); fxp.satDrive = M(ModT::SatDrive); fxp.satTone = juce::jlimit(-1.0f, 1.0f, M(ModT::SatTone)); fxp.satBias = pSatBias->load(); fxp.satLowHz = pSatLow->load(); fxp.satOutDb = pSatOut->load(); fxp.satMix = pSatMix->load();
    fxp.glueOn = pGlueOn->load() > 0.5f; fxp.glueDepth = M(ModT::GlueDepth); fxp.glueTime = pGlueTime->load(); fxp.glueUp = pGlueUp->load(); fxp.glueDown = pGlueDown->load(); fxp.glueMix = pGlueMix->load(); fxp.glueOutDb = pGlueOut->load();
    fxp.chorusOn = pChorusOn->load() > 0.5f; fxp.chorusMode = int(pChorusMode->load()); fxp.chorusRate = pChorusRate->load(); fxp.chorusDepth = juce::jlimit(0.0f, 1.0f, M(ModT::ChorusDepth)); fxp.chorusMix = juce::jlimit(0.0f, 1.0f, M(ModT::ChorusMix));
    fxp.echoOn = pEchoOn->load() > 0.5f; fxp.echoFeedback = M(ModT::EchoFb); fxp.echoMix = M(ModT::EchoMix); fxp.echoTone = M(ModT::EchoTone); fxp.echoWobble = juce::jlimit(0.0f, 1.0f, M(ModT::EchoWobble)); fxp.echoWidth = pEchoWidth->load(); fxp.echoPingPong = pEchoPP->load() > 0.5f;
    fxp.echoTimeMs = pEchoSync->load() > 0.5f ? float(zwui::divisionSeconds(int(pEchoDiv->load()), hostBpm) * 1000.0) : pEchoTime->load();
    fx.set(fxp);
    smVolume.setTarget(zw::dbToGain(pVolume->load()));
}

nebula::Voice* NebulaAudioProcessor::findVoice(int note)
{
    for (auto& v : voices) if (v.active() && v.noteNumber() == note && ! v.isReleased()) return &v;
    for (auto& v : voices) if (! v.active()) return &v;
    Voice* best = nullptr; int bestAge = -1;
    for (auto& v : voices) if (v.isReleased() && v.ageSamples() > bestAge) { best = &v; bestAge = v.ageSamples(); }
    if (best) return best;
    for (auto& v : voices) if (v.ageSamples() > bestAge) { best = &v; bestAge = v.ageSamples(); }
    return best;
}

void NebulaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    hostBpm = zwui::readTransport(*this, motion, hostBpm);
    if (pendingReady.load()) { const juce::SpinLock::ScopedTryLockType sl(tableLock); if (sl.isLocked()) { audioTables[0] = pendingTables[0]; audioTables[1] = pendingTables[1]; pendingReady.store(false); } }

    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer(midi, 0, n, true);
    auto out = getBusBuffer(buffer, false, 0);
    const float* scL = nullptr; const float* scR = nullptr;
    if (getBusCount(true) > 0 && getBus(true, 0)->isEnabled())
    {
        auto sc = getBusBuffer(buffer, true, 0);
        if (sc.getNumChannels() > 0) { scL = sc.getReadPointer(0); scR = sc.getNumChannels() > 1 ? sc.getReadPointer(1) : scL; }
    }
    sidechainPresent.store(scL != nullptr);
    // sidechain is read before we overwrite the buffer (JUCE may alias input/output channels); copy it
    if (scL) { scCopy.setSize(2, n, false, false, true); scCopy.copyFrom(0, 0, scL, n); scCopy.copyFrom(1, 0, scR, n); scL = scCopy.getReadPointer(0); scR = scCopy.getReadPointer(1); }

    float* L = out.getWritePointer(0); float* R = out.getNumChannels() > 1 ? out.getWritePointer(1) : L;
    juce::FloatVectorOperations::clear(L, n); if (R != L) juce::FloatVectorOperations::clear(R, n);

    // MIDI
    int pos = 0;
    auto renderRange = [&](int start, int len)
    {
        if (len <= 0) return;
        constexpr int kSub = 64;
        for (int s0 = start; s0 < start + len; s0 += kSub)
        {
            const int l2 = juce::jmin(kSub, start + len - s0);
            // modulation sources follow the synth's own output (last sub-block) or the sidechain
            mod.process(L + juce::jmax(0, s0 - kSub), R + juce::jmax(0, s0 - kSub), l2, hostBpm, motion.ppq.load(), motion.playing.load(), scL ? scL + s0 : nullptr, scR ? scR + s0 : nullptr);
            updateVoiceParams();
            const float lfo1 = mod.vis[0].load();
            for (auto& v : voices) if (v.active()) v.render(L + s0, R + s0, l2, lfo1, bendSemis, pressure);
        }
    };
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage(); const int t = juce::jlimit(0, n, meta.samplePosition);
        renderRange(pos, t - pos); pos = t;
        if (m.isNoteOn())
        {
            if (auto* v = findVoice(m.getNoteNumber()))
            {
                const float from = (! heldNotes.empty() && pGlide->load() > 0.5f) ? lastNotePitch : -1.0f;
                v->noteOn(m.getNoteNumber(), m.getFloatVelocity(), from, pGlide->load());
            }
            heldNotes.push_back(m.getNoteNumber()); lastNotePitch = float(m.getNoteNumber());
        }
        else if (m.isNoteOff())
        {
            for (auto& v : voices) if (v.active() && v.noteNumber() == m.getNoteNumber() && ! v.isReleased()) v.noteOff();
            heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), m.getNoteNumber()), heldNotes.end());
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) { for (auto& v : voices) v.noteOff(); heldNotes.clear(); }
        else if (m.isPitchWheel()) bendSemis = (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
        else if (m.isChannelPressure()) pressure = m.getChannelPressureValue() / 127.0f;
        else if (m.isAftertouch()) pressure = m.getAfterTouchValue() / 127.0f;
        else if (m.isController() && m.getControllerNumber() == 1) pressure = m.getControllerValue() / 127.0f;   // mod wheel acts like pressure
    }
    renderRange(pos, n - pos);
    midi.clear();

    // internal FX chain, then space + duck + volume
    fx.process(L, R, n);
    {
        const float relK = 1.0f - std::exp(-1.0f / (pDuckRelease->load() * 0.001f * float(currentSampleRate)));
        const float duckAmt = mod.value(ModT::Duck);
        smPower.setTarget(pPower->load() > 0.5f ? 1.0f : 0.0f);
        for (int i = 0; i < n; ++i)
        {
            const float mix = smSpaceMix.next() * smSpaceOn.next(), vol = smVolume.next() * smPower.next(), pre = smSpacePre.next(), sh = smShimmer.next();
            float wl = 0.0f, wr = 0.0f;
            spacePreDl[0].write(L[i]); spacePreDl[1].write(R[i]);
            float inL = pre < 1.0f ? L[i] : spacePreDl[0].read(pre), inR = pre < 1.0f ? R[i] : spacePreDl[1].read(pre);
            if (sh > 0.001f)   // shimmer: octave-up copy of the tail fed back into the input
            {
                inL += shimLP[0].process(shimHP[0].process(shimmer[0].process(lastWetL))) * 0.42f * sh;
                inR += shimLP[1].process(shimHP[1].process(shimmer[1].process(lastWetR))) * 0.42f * sh;
            }
            reverb.process(inL, inR, wl, wr);
            lastWetL = wl; lastWetR = wr;
            { const float w = spaceWidthCur, m = 0.5f * (wl + wr), sd = 0.5f * (wl - wr) * w; wl = m + sd; wr = m - sd; }
            float l = L[i] + wl * mix * 1.2f, r = R[i] + wr * mix * 1.2f;
            float scLevel = 0.0f;
            if (scL) scLevel = std::max(std::fabs(scLP[0].process(scL[i])), std::fabs(scLP[1].process(scR[i])));
            scEnv += (scLevel - scEnv) * (scLevel > scEnv ? 0.3f : relK);
            const float dg = 1.0f - juce::jlimit(0.0f, 1.0f, scEnv * 6.0f) * duckAmt;
            l = zw::softCeiling(l * vol * dg); r = zw::softCeiling(r * vol * dg);
            L[i] = l; R[i] = r;
            if (i == n - 1) duckVis.store(1.0f - dg);
        }
    }
    meterOut.push(out, motion.levelOut);
    spectrum.push(out); stereo.push(out);
    for (int i = 0; i < kVoices; ++i) { voicePitch[size_t(i)].store(voices[i].active() ? voices[i].currentPitch() : -1.0f); voiceLevel[size_t(i)].store(voices[i].ampLevel()); }
    lfo1Vis.store(mod.vis[0].load()); envVis.store(mod.vis[2].load());
}

void NebulaAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= Presets::count()) return;
    currentPreset = index;
    zwui::resetModParams(apvts);
    Presets::apply(index, apvts);
    bank.clearCurrentUser();
    tablesDirty = true;
}

void NebulaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    bank.writeTo(state);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void NebulaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            currentPreset = int(tree.getProperty("preset", 0));
            apvts.replaceState(tree);
            bank.readFrom(tree);
            tablesDirty = true;
        }
}

juce::AudioProcessorEditor* NebulaAudioProcessor::createEditor() { return new NebulaAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NebulaAudioProcessor(); }
