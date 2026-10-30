#pragma once
#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include "PluginProcessor.h"

class DriftAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DriftAudioProcessorEditor(DriftAudioProcessor&);
    ~DriftAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPresetBox();

    DriftAudioProcessor& processor;
    zwui::LookAndFeel lnf { zwui::Palette::violet, zwui::Palette::violetHi, zwui::Palette::cyan };
    zwui::Stage stage;

    zwui::Knob time, feedback, wobble, age, space, mix;          // macro row
    zwui::Knob size, decay, width, spread, output, wobbleRate, color;   // deep row
    zwui::Knob lowCut, highCut, tilt;                              // tone
    juce::ComboBox lowSlopeBox, highSlopeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowSlopeAttach, highSlopeAttach;
    zwui::SpectrumAnalyser analyser;
    zwui::GalaxyScope scope;

    juce::ToggleButton syncButton { "SYNC" }, pingPongButton { "PING-PONG" }, freezeButton { "FREEZE" },
                       wobbleSyncButton { "SYNC" }, powerButton;
    juce::ComboBox divisionBox, wobbleDivBox, presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach, pingAttach, freezeAttach, wobbleSyncAttach, powerAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttach, wobbleDivAttach;
    int lastPreset = -1;
    double lastTick = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftAudioProcessorEditor)
};
