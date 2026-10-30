#pragma once
#include <JuceHeader.h>
#include <zenwave/zenwave.h>
#include <zenwave_ui/ProcessorHelpers.h>
#include <zenwave_ui/Modulation.h>
#include <zenwave_ui/PresetBank.h>
#include "Presets.h"

namespace ParamIDs
{
    inline constexpr auto time       = "time";
    inline constexpr auto sync       = "sync";
    inline constexpr auto division   = "division";
    inline constexpr auto feedback   = "feedback";
    inline constexpr auto wowFlutter = "wowflutter";
    inline constexpr auto age        = "age";
    inline constexpr auto space      = "space";
    inline constexpr auto size       = "size";
    inline constexpr auto decay      = "decay";
    inline constexpr auto width      = "width";
    inline constexpr auto spread     = "spread";
    inline constexpr auto pingPong   = "pingpong";
    inline constexpr auto freeze     = "freeze";
    inline constexpr auto mix        = "mix";
    inline constexpr auto output     = "output";
    inline constexpr auto power      = "power";
    inline constexpr auto wobbleSync = "wobblesync";
    inline constexpr auto wobbleDiv  = "wobblediv";
    inline constexpr auto wobbleRate = "wobblerate";
    inline constexpr auto lowCut     = "lowcut";
    inline constexpr auto highCut    = "highcut";
    inline constexpr auto lowSlope   = "lowslope";
    inline constexpr auto highSlope  = "highslope";
    inline constexpr auto tilt       = "tilt";
    inline constexpr auto color      = "color";
}

// Modulation targets (order = index into the mod matrix; names appear in the routing menus)
namespace ModT { enum { Time, Feedback, Wobble, Age, Space, Size, Decay, Width, Spread, Mix, WobbleRate, LowCut, HighCut, Tilt, Color, Count }; }
inline const std::vector<zwui::ModTarget>& driftModTargets()
{
    static const std::vector<zwui::ModTarget> t {
        { ParamIDs::time, "Time" }, { ParamIDs::feedback, "Feedback" }, { ParamIDs::wowFlutter, "Wobble" }, { ParamIDs::age, "Age" },
        { ParamIDs::space, "Space" }, { ParamIDs::size, "Size" }, { ParamIDs::decay, "Decay" }, { ParamIDs::width, "Width" },
        { ParamIDs::spread, "Spread" }, { ParamIDs::mix, "Mix" }, { ParamIDs::wobbleRate, "Wobble Rate" }, { ParamIDs::lowCut, "Low Cut" },
        { ParamIDs::highCut, "High Cut" }, { ParamIDs::tilt, "Tilt" }, { ParamIDs::color, "Color" } };
    return t;
}

class DriftAudioProcessor : public juce::AudioProcessor
{
public:
    DriftAudioProcessor();
    ~DriftAudioProcessor() override = default;

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
    zwui::SpectrumTap spectrum, spectrumIn;
    zwui::StereoTap stereo;
    zwui::ModMatrix mod;
    zwui::PresetBank bank { apvts, "Drift" };
    double currentSampleRate = 48000.0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateEngineParams();
    float syncedTimeMs() const;

    zw::DriftEngine engine;
    zwui::PowerSwitch power;
    zwui::Meter meterIn, meterOut;
    juce::AudioBuffer<float> monoScratch;
    int currentPreset = 0;
    double hostBpm = 120.0;

    std::atomic<float>* pTime{}, * pSync{}, * pDivision{}, * pFeedback{}, * pWow{}, * pAge{}, * pSpace{},
                      * pSize{}, * pDecay{}, * pWidth{}, * pSpread{}, * pPingPong{}, * pFreeze{}, * pMix{}, * pOutput{},
                      * pPower{}, * pWobbleSync{}, * pWobbleDiv{}, * pWobbleRate{},
                      * pLowCut{}, * pHighCut{}, * pLowSlope{}, * pHighSlope{}, * pTilt{}, * pColor{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftAudioProcessor)
};
