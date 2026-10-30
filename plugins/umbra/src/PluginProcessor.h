#pragma once
#include <JuceHeader.h>
#include <zenwave/zenwave.h>
#include <zenwave_ui/ProcessorHelpers.h>
#include "Presets.h"

namespace ParamIDs
{
    inline constexpr auto preDelay = "predelay";
    inline constexpr auto size     = "size";
    inline constexpr auto decay    = "decay";
    inline constexpr auto dark     = "dark";
    inline constexpr auto density  = "density";
    inline constexpr auto motion   = "motion";
    inline constexpr auto gravity  = "gravity";
    inline constexpr auto early    = "early";
    inline constexpr auto duck     = "duck";
    inline constexpr auto width    = "width";
    inline constexpr auto mix      = "mix";
    inline constexpr auto output   = "output";
    inline constexpr auto freeze   = "freeze";
    inline constexpr auto power    = "power";
    inline constexpr auto motionSync = "motionsync";
    inline constexpr auto motionDiv  = "motiondiv";
    inline constexpr auto motionRate = "motionrate";
    inline constexpr auto lowCut     = "lowcut";
    inline constexpr auto highCut    = "highcut";
    inline constexpr auto lowSlope   = "lowslope";
    inline constexpr auto highSlope  = "highslope";
    inline constexpr auto tilt       = "tilt";
    inline constexpr auto bloom      = "bloom";
}

class UmbraAudioProcessor : public juce::AudioProcessor
{
public:
    UmbraAudioProcessor();
    ~UmbraAudioProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 30.0; }

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
    double currentSampleRate = 48000.0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateEngineParams();

    zw::UmbraEngine engine;
    zwui::PowerSwitch power;
    zwui::Meter meterIn, meterOut;
    int currentPreset = 0;
    double hostBpm = 120.0;
    std::atomic<float>* p[23] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmbraAudioProcessor)
};
