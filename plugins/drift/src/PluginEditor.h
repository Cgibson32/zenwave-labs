#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ZenWave house style: deep-space black, electric violet accent, instrument-panel knobs.
class ZenWaveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour bg        { 0xff0b0b10 };
    static inline const juce::Colour panel     { 0xff14141c };
    static inline const juce::Colour line      { 0xff2a2a38 };
    static inline const juce::Colour text      { 0xffe8e8f0 };
    static inline const juce::Colour dim       { 0xff8a8a9a };
    static inline const juce::Colour violet    { 0xff7c3aed };
    static inline const juce::Colour violetHi  { 0xffa78bfa };

    ZenWaveLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(juce::FontOptions(13.0f)); }
    juce::Font getLabelFont(juce::Label&) override { return juce::Font(juce::FontOptions(12.0f)); }
};

class DriftAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DriftAudioProcessorEditor(DriftAudioProcessor&);
    ~DriftAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    void makeKnob(Knob& k, const char* paramId, const juce::String& title, bool big);
    void timerCallback() override;
    void drawStarfield(juce::Graphics&);
    void refreshPresetBox();

    DriftAudioProcessor& processor;
    ZenWaveLookAndFeel lnf;

    // Macro row (easy)
    Knob time, feedback, wobble, age, space, mix;
    // Deep row (full control)
    Knob size, decay, width, spread, output;

    juce::ToggleButton syncButton { "SYNC" }, pingPongButton { "PING-PONG" }, freezeButton { "FREEZE" };
    juce::ComboBox divisionBox, presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach, pingAttach, freezeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttach;

    std::vector<juce::Point<float>> stars;
    juce::Random rng { 42 };
    int lastPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftAudioProcessorEditor)
};
