#pragma once
#include <JuceHeader.h>
#include <zenwave/zenwave.h>
#include <zenwave_ui/ProcessorHelpers.h>
#include <zenwave_ui/Modulation.h>
#include <zenwave_ui/PresetBank.h>
#include <zenwave_ui/PerformPad.h>
#include "Presets.h"

namespace ParamIDs
{
    inline constexpr auto drive   = "drive";
    inline constexpr auto heat    = "heat";
    inline constexpr auto focus   = "focus";
    inline constexpr auto width   = "width";
    inline constexpr auto mix     = "mix";
    inline constexpr auto output  = "output";
    inline constexpr auto lowX    = "lowx";
    inline constexpr auto highX   = "highx";
    inline constexpr auto subLock = "sublock";
    inline constexpr auto power   = "power";
    inline constexpr auto lowCut    = "lowcut";
    inline constexpr auto highCut   = "highcut";
    inline constexpr auto lowSlope  = "lowslope";
    inline constexpr auto highSlope = "highslope";
    inline constexpr auto tilt      = "tilt";
    inline juce::String bandChar (int b) { return "b" + juce::String(b + 1) + "char"; }
    inline juce::String bandDrive(int b) { return "b" + juce::String(b + 1) + "drive"; }
    inline juce::String bandMix  (int b) { return "b" + juce::String(b + 1) + "mix"; }
    inline juce::String bandGain (int b) { return "b" + juce::String(b + 1) + "gain"; }
    inline juce::String bandOn   (int b) { return "b" + juce::String(b + 1) + "on"; }
    inline juce::String bandSolo (int b) { return "b" + juce::String(b + 1) + "solo"; }
    inline juce::String bandTune (int b) { return "b" + juce::String(b + 1) + "tune"; }
    inline juce::String bandRes  (int b) { return "b" + juce::String(b + 1) + "res"; }
    inline constexpr auto crushOn   = "crushon";
    inline constexpr auto crushBits = "crushbits";
    inline constexpr auto crushRate = "crushrate";
    inline constexpr auto crushMix  = "crushmix";
    inline constexpr auto vowel     = "vowel";
    inline constexpr auto punch     = "punch";
}

namespace ModT { enum { Drive, Heat, Focus, Width, Mix, LowX, HighX, LowDrive, MidDrive, HighDrive, LowCut, HighCut, Tilt,
                        LowTune, MidTune, HighTune, LowRes, MidRes, HighRes, CrushBits, CrushRate, Punch, Count }; }
inline const std::vector<zwui::ModTarget>& coreModTargets()
{
    static const std::vector<zwui::ModTarget> t {
        { ParamIDs::drive, "Drive" }, { ParamIDs::heat, "Heat" }, { ParamIDs::focus, "Focus" }, { ParamIDs::width, "Width" }, { ParamIDs::mix, "Mix" },
        { ParamIDs::lowX, "Low X" }, { ParamIDs::highX, "High X" }, { "b1drive", "Low Drive" }, { "b2drive", "Mid Drive" }, { "b3drive", "High Drive" },
        { ParamIDs::lowCut, "Low Cut" }, { ParamIDs::highCut, "High Cut" }, { ParamIDs::tilt, "Tilt" },
        { "b1tune", "Low Tune" }, { "b2tune", "Mid Tune" }, { "b3tune", "High Tune" }, { "b1res", "Low Res" }, { "b2res", "Mid Res" }, { "b3res", "High Res" },
        { ParamIDs::crushBits, "Crush Bits" }, { ParamIDs::crushRate, "Crush Rate" }, { ParamIDs::punch, "Punch" } };
    return t;
}

class CoreAudioProcessor : public juce::AudioProcessor
{
public:
    CoreAudioProcessor();
    ~CoreAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return Presets::count(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override { return Presets::name(index); }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    zwui::Motion motion;
    zwui::SpectrumTap spectrum, spectrumIn;
    zwui::StereoTap stereo;
    zwui::ModMatrix mod;
    zwui::PadEngine pad;                   // X = drive, Y = mix
    zwui::PresetBank bank { apvts, "Core" };
    juce::AudioBuffer<float> wetScratch;
    double currentSampleRate = 48000.0;
    std::array<std::atomic<float>, 3> bandGainDb {};   // auto-gain readout per band (UI)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateEngineParams();

    zw::CoreEngine engine;
    zwui::PowerSwitch power;
    zwui::Meter meterIn, meterOut;
    int currentPreset = 0;
    double hostBpm = 120.0;

    std::atomic<float>* pOutput{}, * pSubLock{}, * pPower{}, * pLowSlope{}, * pHighSlope{};
    std::atomic<float>* pBandChar[3]{}, * pBandMix[3]{}, * pBandGain[3]{}, * pBandOn[3]{}, * pBandSolo[3]{};
    std::atomic<float>* pCrushOn{}, * pCrushMix{}, * pVowel{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoreAudioProcessor)
};
