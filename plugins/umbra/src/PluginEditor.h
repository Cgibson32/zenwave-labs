#pragma once
#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include <zenwave_ui/ModPanel.h>
#include <zenwave_ui/PresetBar.h>
#include "Manual.h"
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
    void installLocks();
    void syncModRings();

    UmbraAudioProcessor& processor;
    // Umbra's accent is a deeper, colder violet than Drift's
    zwui::LookAndFeel lnf { juce::Colour(0xff7c3aed), juce::Colour(0xffc4b5fd), zwui::Palette::magenta };
    zwui::Stage stage;

    zwui::Knob preDelay, size, decay, dark, mix;            // macro row
    zwui::Knob density, motion, gravity, early, duck, width, output, motionRate, bloom;  // deep row
    zwui::Knob lowCut, highCut, tilt;
    juce::ComboBox lowSlopeBox, highSlopeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowSlopeAttach, highSlopeAttach;
    zwui::SpectrumAnalyser analyser, analyserIn;
    zwui::GalaxyScope scope;
    zwui::ModPanel modPanel { processor.apvts, processor.mod, lnf };
    zwui::PresetBar presetBar;

    juce::ToggleButton freezeButton { "FREEZE" }, motionSyncButton { "SYNC" }, powerButton;
    juce::ComboBox motionDivBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttach, motionSyncAttach, powerAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> motionDivAttach;
    double lastTick = 0.0;
    std::vector<zwui::Knob*> allKnobs;
    juce::TextButton helpButton { "?" };
    juce::TooltipWindow tooltips { this, 650 };
    zwui::ManualOverlay manual { lnf };   // last: sits on top of everything

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmbraAudioProcessorEditor)
};
