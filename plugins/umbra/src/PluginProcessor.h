#pragma once
#include <JuceHeader.h>
#include <zenwave/zenwave.h>
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateEngineParams();

    zw::UmbraEngine engine;
    int currentPreset = 0;
    std::atomic<float>* p[13] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmbraAudioProcessor)
};
