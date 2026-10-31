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
    inline constexpr auto in        = "in";
    inline constexpr auto depth     = "depth";
    inline constexpr auto time      = "time";
    inline constexpr auto autoMode  = "auto";
    inline constexpr auto subLock   = "sublock";
    inline constexpr auto grit      = "grit";
    inline constexpr auto shapeAtk  = "shapeatk";
    inline constexpr auto shapeSus  = "shapesus";
    inline constexpr auto duck      = "duck";
    inline constexpr auto scAmount  = "scamount";
    inline constexpr auto scRelease = "screlease";
    inline constexpr auto lowX      = "lowx";
    inline constexpr auto highX     = "highx";
    inline constexpr auto mix       = "mix";
    inline constexpr auto output    = "output";
    inline constexpr auto power     = "power";
    inline juce::String bandOn   (int b) { return "b" + juce::String(b + 1) + "on"; }
    inline juce::String bandSolo (int b) { return "b" + juce::String(b + 1) + "solo"; }
    inline juce::String bandDepth(int b) { return "b" + juce::String(b + 1) + "depth"; }
    inline juce::String bandGain (int b) { return "b" + juce::String(b + 1) + "gain"; }
    inline juce::String bandAtk  (int b) { return "b" + juce::String(b + 1) + "atk"; }
    inline juce::String bandRel  (int b) { return "b" + juce::String(b + 1) + "rel"; }
    inline juce::String nodeOn   (int i) { return "n" + juce::String(i + 1) + "on"; }
    inline juce::String nodeFreq (int i) { return "n" + juce::String(i + 1) + "freq"; }
    inline juce::String nodeQ    (int i) { return "n" + juce::String(i + 1) + "q"; }
    inline juce::String nodeThr  (int i) { return "n" + juce::String(i + 1) + "thr"; }
    inline juce::String nodeRange(int i) { return "n" + juce::String(i + 1) + "range"; }
}

namespace ModT { enum { Depth, Time, Mix, Duck, ShapeAtk, ShapeSus, LowX, HighX, LowDepth, MidDepth, HighDepth,
                        N1Freq, N2Freq, N3Freq, N4Freq, N1Range, N2Range, N3Range, N4Range, Count }; }
inline const std::vector<zwui::ModTarget>& pulseModTargets()
{
    static const std::vector<zwui::ModTarget> t {
        { ParamIDs::depth, "Depth" }, { ParamIDs::time, "Time" }, { ParamIDs::mix, "Mix" }, { ParamIDs::duck, "Duck" },
        { ParamIDs::shapeAtk, "Attack" }, { ParamIDs::shapeSus, "Sustain" }, { ParamIDs::lowX, "Low X" }, { ParamIDs::highX, "High X" },
        { "b1depth", "Low Depth" }, { "b2depth", "Mid Depth" }, { "b3depth", "High Depth" },
        { "n1freq", "Node 1 Freq" }, { "n2freq", "Node 2 Freq" }, { "n3freq", "Node 3 Freq" }, { "n4freq", "Node 4 Freq" },
        { "n1range", "Node 1 Range" }, { "n2range", "Node 2 Range" }, { "n3range", "Node 3 Range" }, { "n4range", "Node 4 Range" } };
    return t;
}

class PulseAudioProcessor : public juce::AudioProcessor
{
public:
    PulseAudioProcessor();
    ~PulseAudioProcessor() override = default;

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
    zwui::PadEngine pad;                   // X = depth, Y = duck
    zwui::PresetBank bank { apvts, "Pulse" };
    juce::AudioBuffer<float> wetScratch;
    double currentSampleRate = 48000.0;
    // live readouts for the editor
    std::array<std::atomic<float>, 3> bandGr {};
    std::array<std::atomic<float>, 4> nodeGain {};
    std::atomic<float> transientVis { 0.0f }, duckVis { 0.0f }, inRms { 0.0f }, outRms { 0.0f };
    std::atomic<bool> sidechainPresent { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateEngineParams();

    zw::PulseEngine engine;
    zwui::PowerSwitch power;
    zwui::Meter meterIn, meterOut;
    int currentPreset = 0;
    double hostBpm = 120.0;

    std::atomic<float>* pIn{}, * pOutput{}, * pAuto{}, * pSubLock{}, * pGrit{}, * pPower{}, * pScAmount{}, * pScRelease{};
    std::atomic<float>* pBandOn[3]{}, * pBandSolo[3]{}, * pBandGain[3]{}, * pBandAtk[3]{}, * pBandRel[3]{};
    std::atomic<float>* pNodeOn[4]{}, * pNodeQ[4]{}, * pNodeThr[4]{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseAudioProcessor)
};
