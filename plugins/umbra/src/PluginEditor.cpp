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
    motionRate.init(*this, processor.apvts, ParamIDs::motionRate, "MOTION RATE", false);
    lowCut.init (*this, processor.apvts, ParamIDs::lowCut,  "LOW CUT",  false);
    highCut.init(*this, processor.apvts, ParamIDs::highCut, "HIGH CUT", false);
    tilt.init   (*this, processor.apvts, ParamIDs::tilt,    "TILT",     false);
    for (auto* box : { &lowSlopeBox, &highSlopeBox }) { addAndMakeVisible(box); box->addItemList({ "6 dB", "12 dB", "24 dB", "48 dB" }, 1); }
    lowSlopeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::lowSlope, lowSlopeBox);
    highSlopeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::highSlope, highSlopeBox);

    for (auto* b : { &freezeButton, &motionSyncButton, &powerButton }) addAndMakeVisible(b);
    powerButton.getProperties().set("round", true);
    freezeAttach     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::freeze, freezeButton);
    motionSyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::motionSync, motionSyncButton);
    powerAttach      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::power, powerButton);
    addAndMakeVisible(motionDivBox);
    motionDivBox.addItemList(zwui::divisionNames(), 1);
    motionDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::motionDiv, motionDivBox);

    addAndMakeVisible(presetBox);
    refreshPresetBox();
    presetBox.onChange = [this] {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) processor.setCurrentProgram(idx);
    };

    setResizable(true, true);
    setResizeLimits(704, 560, 1408, 1120);
    getConstrainer()->setFixedAspectRatio(880.0 / 700.0);
    setSize(880, 700);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
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
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit(0.0f, 0.1f, float((now - lastTick) * 0.001));
    lastTick = now;
    stage.tick(processor.motion, dt);
    lnf.pulse = processor.motion.pulse;
    analyser.update(processor.spectrum, processor.currentSampleRate);
    scope.update(processor.stereo);

    if (processor.getCurrentProgram() != lastPreset)
    {
        lastPreset = processor.getCurrentProgram();
        presetBox.setSelectedId(lastPreset + 1, juce::dontSendNotification);
    }
    const bool msync = motionSyncButton.getToggleState();
    motionDivBox.setEnabled(msync); motionDivBox.setAlpha(msync ? 1.0f : 0.4f);
    motionRate.slider.setAlpha(msync ? 0.35f : 1.0f);
    const bool on = powerButton.getToggleState();
    for (auto* k : { &preDelay, &size, &decay, &dark, &mix, &density, &motion, &gravity, &early, &duck, &width, &output, &motionRate })
        k->slider.setAlpha(on ? k->slider.getAlpha() : 0.3f);
    repaint();
}

void UmbraAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getWidth() / 880.0f;
    stage.render(g, getLocalBounds(), lnf.accent, lnf.accent2, processor.motion, 7);
    zwui::drawHeader(g, r, s, "UMBRA", "DARK REVERB", lnf.accent, lnf.accentHi, lnf.accent2);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 84 * s, 836 * s, 262 * s), "SPACE", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 358 * s, 836 * s, 160 * s), "SHAPE", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 530 * s, 836 * s, 158 * s), "FIELD", lnf.accent, lnf.accent2, s);
    scope.draw(g, juce::Rectangle<float>(40 * s, 548 * s, 128 * s, 128 * s), lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::EqCurve eq { float(lowCut.slider.getValue()), float(highCut.slider.getValue()),
                       juce::jmax(0, lowSlopeBox.getSelectedId() - 1), juce::jmax(0, highSlopeBox.getSelectedId() - 1), float(tilt.slider.getValue()) };
    zwui::drawEqField(g, juce::Rectangle<float>(186 * s, 552 * s, 360 * s, 120 * s), analyser, eq, lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::drawOrbit(g, { 470 * s, 44 * s }, 26 * s, processor.motion, lnf.accent, lnf.accent2);
}

void UmbraAudioProcessorEditor::resized()
{
    const float s = float(getWidth()) / 880.0f;
    auto S = [s](int v) { return juce::roundToInt(v * s); };

    powerButton.setBounds(S(400), S(22), S(34), S(34));
    presetBox.setBounds(S(520), S(24), S(250), S(30));
    freezeButton.setBounds(S(780), S(24), S(78), S(30));

    zwui::Knob* bigs[] = { &preDelay, &size, &decay, &dark, &mix };
    int x = S(66);
    for (auto* k : bigs) { k->layout(x, S(120), S(140), S(160), S(16)); x += S(156); }

    motionSyncButton.setBounds(S(52), S(300), S(58), S(26));
    motionDivBox.setBounds(S(116), S(300), S(84), S(26));

    zwui::Knob* smalls[] = { &density, &motion, &motionRate, &gravity, &early, &duck, &width, &output };
    x = S(34);
    for (auto* k : smalls) { k->layout(x, S(392), S(96), S(110), S(14)); x += S(103); }

    zwui::Knob* tone[] = { &lowCut, &highCut, &tilt };
    x = S(566);
    for (auto* k : tone) { k->layout(x, S(572), S(90), S(84), S(12)); x += S(98); }
    lowSlopeBox.setBounds(S(574), S(662), S(74), S(18));
    highSlopeBox.setBounds(S(672), S(662), S(74), S(18));
}
