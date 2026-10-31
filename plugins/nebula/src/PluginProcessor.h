#pragma once
#include <JuceHeader.h>
#include <zenwave/zenwave.h>
#include <zenwave_ui/ProcessorHelpers.h>
#include <zenwave_ui/Modulation.h>
#include <zenwave_ui/PresetBank.h>
#include "Engine.h"
#include "Presets.h"

namespace ParamIDs
{
    inline juce::String osc(int o, const char* p) { return juce::String(o == 0 ? "a" : "b") + p; }   // atable, aposition, alevel, aoctave, asemis, afine, aunison, aspread, awidth, aon
    inline constexpr auto carveOn   = "carveon";
    inline juce::String carveBandOn    (int i) { return "c" + juce::String(i + 1) + "on"; }
    inline juce::String carveBandCentre(int i) { return "c" + juce::String(i + 1) + "centre"; }
    inline juce::String carveBandWidth (int i) { return "c" + juce::String(i + 1) + "width"; }
    inline juce::String carveBandGain  (int i) { return "c" + juce::String(i + 1) + "gain"; }
    inline constexpr auto cutoff    = "cutoff";
    inline constexpr auto res       = "res";
    inline constexpr auto filterMode = "filtermode";
    inline constexpr auto envAmt    = "envamt";
    inline constexpr auto keyTrack  = "keytrack";
    inline constexpr auto drive     = "drive";
    inline constexpr auto drift     = "drift";
    inline constexpr auto glide     = "glide";
    inline constexpr auto ampA = "ampa", ampD = "ampd", ampS = "amps", ampR = "ampr";
    inline constexpr auto modA = "moda", modD = "modd", modS = "mods", modR = "modr";
    inline constexpr auto spaceMix = "spacemix", spaceSize = "spacesize", spaceDecay = "spacedecay", spaceDark = "spacedark";
    inline constexpr auto mWarmth = "mwarmth", mAir = "mair", mMovement = "mmovement", mSpace = "mspace", mGrit = "mgrit", mWidth = "mwidth", mAttack = "mattack", mRelease = "mrelease";
    inline constexpr auto duck = "duck";          // sidechain duck amount
    inline constexpr auto duckRelease = "duckrelease";
    inline constexpr auto volume = "volume";
    inline constexpr auto power  = "power";
}

namespace ModT { enum { PosA, PosB, Cutoff, Res, Drive, LevelA, LevelB, SpaceMix, Warmth, Air, Movement, Grit, Width, Drift, Duck, Count }; }
inline const std::vector<zwui::ModTarget>& nebulaModTargets()
{
    static const std::vector<zwui::ModTarget> t {
        { "aposition", "Position A" }, { "bposition", "Position B" }, { ParamIDs::cutoff, "Cutoff" }, { ParamIDs::res, "Resonance" }, { ParamIDs::drive, "Drive" },
        { "alevel", "Level A" }, { "blevel", "Level B" }, { ParamIDs::spaceMix, "Space" }, { ParamIDs::mWarmth, "Warmth" }, { ParamIDs::mAir, "Air" },
        { ParamIDs::mMovement, "Movement" }, { ParamIDs::mGrit, "Grit" }, { ParamIDs::mWidth, "Width" }, { ParamIDs::drift, "Drift" }, { ParamIDs::duck, "Duck" } };
    return t;
}

class NebulaAudioProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    static constexpr int kVoices = 16;
    NebulaAudioProcessor();
    ~NebulaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return Presets::count(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override { return Presets::name(index); }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    zwui::Motion motion;
    zwui::SpectrumTap spectrum;
    zwui::StereoTap stereo;
    zwui::ModMatrix mod;
    zwui::PresetBank bank { apvts, "Nebula" };
    double currentSampleRate = 48000.0;

    // For the planet display (message thread reads the shared_ptrs; audio thread holds its own copies)
    std::shared_ptr<nebula::Table> tableForUI(int osc) const { return uiTables[osc]; }
    std::array<nebula::CarveBand, nebula::kCarveBands> carveBands() const;
    struct VoiceInfo { float pitch; float level; bool released; };
    std::array<std::atomic<float>, kVoices> voicePitch {}, voiceLevel {};
    std::atomic<float> lfo1Vis { 0.0f }, envVis { 0.0f };
    void markTablesDirty() { tablesDirty = true; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateVoiceParams();
    void rebuildTablesIfNeeded();
    void timerCallback() override { rebuildTablesIfNeeded(); }
    nebula::Voice* findVoice(int note);

    nebula::TableRenderer renderer;
    std::shared_ptr<nebula::Table> uiTables[2];
    std::shared_ptr<nebula::Table> audioTables[2];
    std::shared_ptr<nebula::Table> pendingTables[2];
    std::atomic<bool> tablesDirty { true }, pendingReady { false };
    int renderedTable[2] = { -1, -1 }; std::array<nebula::CarveBand, nebula::kCarveBands> renderedCarve {}; bool renderedCarveOn = false;

    nebula::VoiceParams vp;
    nebula::Voice voices[kVoices];
    zw::SpaceReverb reverb;
    zw::Smoother smSpaceMix, smVolume;
    zw::Smoother smPower;
    zwui::Meter meterOut;
    juce::AudioBuffer<float> wet, scCopy;
    std::unique_ptr<juce::AudioProcessorValueTreeState::Listener> tableListener;
    juce::SpinLock tableLock;
    int currentPreset = 0; double hostBpm = 120.0;
    float bendSemis = 0.0f, pressure = 0.0f, lastNotePitch = -1.0f;
    std::vector<int> heldNotes;

    std::atomic<float>* pTable[2]{}, * pPos[2]{}, * pLevel[2]{}, * pOct[2]{}, * pSemi[2]{}, * pFine[2]{}, * pUni[2]{}, * pSpread[2]{}, * pWidth[2]{}, * pOn[2]{};
    std::atomic<float>* pCOn[nebula::kCarveBands]{}, * pCCentre[nebula::kCarveBands]{}, * pCWidth[nebula::kCarveBands]{}, * pCGain[nebula::kCarveBands]{}, * pCarveOn{};
    std::atomic<float>* pCutoff{}, * pRes{}, * pFilterMode{}, * pEnvAmt{}, * pKeyTrack{}, * pDrive{}, * pDrift{}, * pGlide{};
    std::atomic<float>* pAmp[4]{}, * pModEnv[4]{};
    std::atomic<float>* pSpaceMix{}, * pSpaceSize{}, * pSpaceDecay{}, * pSpaceDark{};
    std::atomic<float>* pMacro[8]{}; std::atomic<float>* pVolume{}, * pPower{}, * pDuck{}, * pDuckRelease{};
    float scEnv = 0.0f; zw::OnePole scLP[2];
public:
    std::atomic<bool> sidechainPresent { false };
    std::atomic<float> duckVis { 0.0f };
    juce::MidiKeyboardState keyboardState;   // on-screen keyboard in the editor

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NebulaAudioProcessor)
};
