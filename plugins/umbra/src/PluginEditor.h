#pragma once
#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include "PluginProcessor.h"

class UmbraAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit UmbraAudioProcessorEditor(UmbraAudioProcessor&);
    ~UmbraAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPresetBox();

    UmbraAudioProcessor& processor;
    // Umbra's accent is a deeper, colder violet than Drift's
    zwui::LookAndFeel lnf { juce::Colour(0xff6d28d9), juce::Colour(0xff9f7aea) };

    zwui::Knob preDelay, size, decay, dark, mix;            // macro row
    zwui::Knob density, motion, gravity, early, duck, width, output;  // deep row

    juce::ToggleButton freezeButton { "FREEZE" };
    juce::ComboBox presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttach;
    int lastPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmbraAudioProcessorEditor)
};
