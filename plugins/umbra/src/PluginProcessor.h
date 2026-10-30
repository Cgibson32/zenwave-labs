#pragma once
#include <JuceHeader.h>
#include <zenwave/zenwave.h>
#include <zenwave_ui/ProcessorHelpers.h>
#include <zenwave_ui/Modulation.h>
#include <zenwave_ui/PresetBank.h>
#include <zenwave_ui/Orbit.h>
#include <zenwave_ui/PerformPad.h>
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

namespace ModT { enum { PreDelay, Size, Decay, Dark, Density, Motion, Gravity, Early, Duck, Width, Mix, MotionRate, LowCut, HighCut, Tilt, Bloom, Count }; }
inline const std::vector<zwui::ModTarget>& umbraModTargets()
{
    static const std::vector<zwui::ModTarget> t {
        { ParamIDs::preDelay, "Pre-Delay" }, { ParamIDs::size, "Size" }, { ParamIDs::decay, "Decay" }, { ParamIDs::dark, "Dark" },
        { ParamIDs::density, "Density" }, { ParamIDs::motion, "Motion" }, { ParamIDs::gravity, "Gravity" }, { ParamIDs::early, "Early" },
        { ParamIDs::duck, "Duck" }, { ParamIDs::width, "Width" }, { ParamIDs::mix, "Mix" }, { ParamIDs::motionRate, "Motion Rate" },
        { ParamIDs::lowCut, "Low Cut" }, { ParamIDs::highCut, "High Cut" }, { ParamIDs::tilt, "Tilt" }, { ParamIDs::bloom, "Bloom" } };
    return t;
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
    zwui::SpectrumTap spectrum, spectrumIn;
    zwui::StereoTap stereo;
    zwui::ModMatrix mod;
    zwui::OrbitLink orbitLink { apvts, ParamIDs::freeze };
    zwui::PadEngine pad;   // X = decay, Y = mix (a reverb "throw")
    juce::AudioBuffer<float> wetScratch;   // post-EQ wet signal for the Field analyser
    zwui::PresetBank bank { apvts, "Umbra" };
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
