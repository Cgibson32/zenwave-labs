#include "PluginEditor.h"

UmbraAudioProcessorEditor::UmbraAudioProcessorEditor(UmbraAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);

    preDelay.init(*this, processor.apvts, ParamIDs::preDelay, "PRE-DELAY", true);
    size.init    (*this, processor.apvts, ParamIDs::size,     "SIZE",      true);
    decay.init   (*this, processor.apvts, ParamIDs::decay,    "DECAY",     true);
    dark.init    (*this, processor.apvts, ParamIDs::dark,     "DARK",      true);
    mix.init     (*this, processor.apvts, ParamIDs::mix,      "MIX",       true);

    density.init (*this, processor.apvts, ParamIDs::density,  "DENSITY",   false);
    motion.init  (*this, processor.apvts, ParamIDs::motion,   "MOTION",    false);
    gravity.init (*this, processor.apvts, ParamIDs::gravity,  "GRAVITY",   false);
    early.init   (*this, processor.apvts, ParamIDs::early,    "EARLY",     false);
    duck.init    (*this, processor.apvts, ParamIDs::duck,     "DUCK",      false);
    width.init   (*this, processor.apvts, ParamIDs::width,    "WIDTH",     false);
    output.init  (*this, processor.apvts, ParamIDs::output,   "OUTPUT",    false);

    addAndMakeVisible(freezeButton);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::freeze, freezeButton);

    addAndMakeVisible(presetBox);
    refreshPresetBox();
    presetBox.onChange = [this] {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) processor.setCurrentProgram(idx);
    };

    setResizable(true, true);
    setResizeLimits(640, 400, 1280, 800);
    getConstrainer()->setFixedAspectRatio(1.6);
    setSize(800, 500);
    startTimerHz(10);
}

UmbraAudioProcessorEditor::~UmbraAudioProcessorEditor() { setLookAndFeel(nullptr); }

void UmbraAudioProcessorEditor::refreshPresetBox()
{
    presetBox.clear(juce::dontSendNotification);
    juce::String lastCat;
    for (int i = 0; i < Presets::count(); ++i)
    {
        const auto cat = Presets::category(i);
        if (cat != lastCat) { if (i > 0) presetBox.addSeparator(); presetBox.addSectionHeading(cat); lastCat = cat; }
        presetBox.addItem(Presets::name(i), i + 1);
    }
    presetBox.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    lastPreset = processor.getCurrentProgram();
}

void UmbraAudioProcessorEditor::timerCallback()
{
    if (processor.getCurrentProgram() != lastPreset)
    {
        lastPreset = processor.getCurrentProgram();
        presetBox.setSelectedId(lastPreset + 1, juce::dontSendNotification);
    }
}

void UmbraAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(zwui::Palette::bg);
    auto r = getLocalBounds().toFloat();
    zwui::drawStarfield(g, r, lnf.accent, 7, 110);
    const float s = r.getWidth() / 800.0f;
    zwui::drawHeader(g, r, s, "UMBRA", "DARK REVERB   ·   ZENWAVE LABS");
    g.setColour(zwui::Palette::line);
    g.drawLine(20 * s, 332 * s, r.getWidth() - 20 * s, 332 * s, 1.0f);
    g.setColour(zwui::Palette::dim);
    g.setFont(juce::Font(juce::FontOptions(10.0f * s, juce::Font::bold)));
    g.drawText("DEEP CONTROL", juce::Rectangle<float>(22 * s, 338 * s, 200 * s, 14 * s), juce::Justification::centredLeft);
}

void UmbraAudioProcessorEditor::resized()
{
    const float s = float(getWidth()) / 800.0f;
    auto S = [s](int v) { return juce::roundToInt(v * s); };

    presetBox.setBounds(S(470), S(22), S(250), S(28));
    freezeButton.setBounds(S(728), S(22), S(52), S(28));

    // Macro row: five big knobs
    zwui::Knob* bigs[] = { &preDelay, &size, &decay, &dark, &mix };
    int x = S(40);
    for (auto* k : bigs) { k->layout(x, S(100), S(132), S(160), S(16)); x += S(148); }

    // Deep row: seven small knobs
    zwui::Knob* smalls[] = { &density, &motion, &gravity, &early, &duck, &width, &output };
    x = S(22);
    for (auto* k : smalls) { k->layout(x, S(368), S(100), S(110), S(14)); x += S(108); }
}
