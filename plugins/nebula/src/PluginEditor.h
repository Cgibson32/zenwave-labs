#pragma once
#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include <zenwave_ui/ModPanel.h>
#include <zenwave_ui/PresetBar.h>
#include "Manual.h"
#include "PluginProcessor.h"
#include "Planet.h"

class NebulaAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NebulaAudioProcessorEditor(NebulaAudioProcessor&);
    ~NebulaAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void installLocks();
    void syncModRings();

    NebulaAudioProcessor& processor;
    // Nebula: deep indigo with a nova-gold secondary - an instrument, not an effect
    zwui::LookAndFeel lnf { juce::Colour(0xff6d5dfc), juce::Colour(0xffd9d4ff), juce::Colour(0xffffc86b) };
    zwui::Stage stage;
    PlanetView planet { processor, lnf };

    struct OscUI { juce::ComboBox table; juce::ToggleButton on; zwui::Knob position, level, octave, semis, fine, unison, spread, width;
                   std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tableAttach; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttach; } osc[2];
    zwui::Knob cutoff, res, envAmt, keyTrack, drive;
    juce::ComboBox filterModeBox; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterModeAttach;
    juce::ToggleButton carveButton { "CARVE" }; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> carveAttach;
    zwui::Knob ampK[4], modK[4];
    zwui::Knob spaceSize, spaceDecay, spaceDark, duck, duckRel, glide, drift, volume;
    zwui::Knob macro[8];
    zwui::Knob vibrato, breath;
    // ---- FX page ----
    int page = 0;                                   // 0 = VOICE, 1 = FX
    juce::TextButton voiceTab { "VOICE" }, fxTab { "FX" };
    void setPage(int p);
    int selectedEq = 1;
    juce::TextButton eqSel[4];
    juce::ToggleButton eqOnButton; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> eqOnAttach;
    juce::ComboBox eqTypeBox; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> eqTypeAttach;
    zwui::Knob eqFreq, eqGain, eqQ;
    void attachEq(int i);
    zwui::SpectrumAnalyser fxAnalyser;
    juce::Rectangle<int> eqDisplay;
    int eqDrag = -1;
    void mouseDown(const juce::MouseEvent&) override; void mouseDrag(const juce::MouseEvent&) override; void mouseUp(const juce::MouseEvent&) override;
    juce::ToggleButton satButton { "SATURATE" }, glueButton { "GLUE" }, echoButton { "ECHO" }, echoSyncButton { "SYNC" }, echoPPButton { "PING-PONG" }, spaceButton { "SPACE" }, shimmerButton { "SHIMMER" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> satAttach, glueAttach, echoAttach, echoSyncAttach, echoPPAttach, spaceAttach, shimmerAttach;
    juce::ComboBox echoDivBox, satTypeBox; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> echoDivAttach, satTypeAttach;
    zwui::Knob satDrive, satTone, satMix, satBias, satLow, satOut, glueDepth, glueTime, glueMix, glueUp, glueDown, glueOut,
               echoTime, echoFb, echoMix, echoTone, echoWobble, echoWidth, spaceMix, spacePre, spaceLow;
    juce::ToggleButton powerButton; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttach;
    zwui::ModPanel modPanel { processor.apvts, processor.mod, lnf };
    zwui::PresetBar presetBar;
    double lastTick = 0.0;
    std::vector<zwui::Knob*> allKnobs;
    juce::TextButton helpButton { "MANUAL" };
    juce::TooltipWindow tooltips { this, 650 };
    zwui::ManualOverlay manual { lnf };
    juce::MidiKeyboardComponent keyboard { processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NebulaAudioProcessorEditor)
};
