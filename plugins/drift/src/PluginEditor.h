#pragma once
#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include <zenwave_ui/ModPanel.h>
#include <zenwave_ui/PresetBar.h>
#include "Manual.h"
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
    void installLocks();
    void syncModRings();

    DriftAudioProcessor& processor;
    zwui::LookAndFeel lnf { zwui::Palette::violet, zwui::Palette::violetHi, zwui::Palette::cyan };
    zwui::Stage stage;

    zwui::Knob time, feedback, wobble, age, space, mix;          // macro row
    zwui::Knob size, decay, width, spread, output, wobbleRate, color;   // deep row
    zwui::Knob lowCut, highCut, tilt;                              // tone
    juce::ComboBox lowSlopeBox, highSlopeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowSlopeAttach, highSlopeAttach;
    zwui::SpectrumAnalyser analyser, analyserIn;
    zwui::GalaxyScope scope;
    zwui::ModPanel modPanel { processor.apvts, processor.mod, lnf };
    zwui::PresetBar presetBar;

    juce::ToggleButton syncButton { "SYNC" }, pingPongButton { "PING-PONG" }, freezeButton { "FREEZE" },
                       wobbleSyncButton { "SYNC" }, powerButton;
    juce::ComboBox divisionBox, wobbleDivBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach, pingAttach, freezeAttach, wobbleSyncAttach, powerAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttach, wobbleDivAttach;
    double lastTick = 0.0;
    std::vector<zwui::Knob*> allKnobs;
    juce::TextButton helpButton { "MANUAL" };
    juce::TooltipWindow tooltips { this, 650 };
    zwui::ManualOverlay manual { lnf };   // last: sits on top of everything

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftAudioProcessorEditor)
};
