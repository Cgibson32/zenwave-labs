#include "PluginEditor.h"

DriftAudioProcessorEditor::DriftAudioProcessorEditor(DriftAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);

    time.init    (*this, processor.apvts, ParamIDs::time,       "TIME",     true);
    feedback.init(*this, processor.apvts, ParamIDs::feedback,   "FEEDBACK", true);
    wobble.init  (*this, processor.apvts, ParamIDs::wowFlutter, "WOBBLE",   true);
    age.init     (*this, processor.apvts, ParamIDs::age,        "AGE",      true);
    space.init   (*this, processor.apvts, ParamIDs::space,      "SPACE",    true);
    mix.init     (*this, processor.apvts, ParamIDs::mix,        "MIX",      true);

    size.init      (*this, processor.apvts, ParamIDs::size,       "SIZE",   false);
    decay.init     (*this, processor.apvts, ParamIDs::decay,      "DECAY",  false);
    width.init     (*this, processor.apvts, ParamIDs::width,      "WIDTH",  false);
    spread.init    (*this, processor.apvts, ParamIDs::spread,     "SPREAD", false);
    wobbleRate.init(*this, processor.apvts, ParamIDs::wobbleRate, "WOBBLE RATE", false);
    output.init    (*this, processor.apvts, ParamIDs::output,     "OUTPUT", false);

    for (auto* b : { &syncButton, &pingPongButton, &freezeButton, &wobbleSyncButton, &powerButton }) addAndMakeVisible(b);
    powerButton.getProperties().set("round", true);
    syncAttach       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::sync, syncButton);
    pingAttach       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::pingPong, pingPongButton);
    freezeAttach     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::freeze, freezeButton);
    wobbleSyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::wobbleSync, wobbleSyncButton);
    powerAttach      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::power, powerButton);

    for (auto* box : { &divisionBox, &wobbleDivBox }) { addAndMakeVisible(box); box->addItemList(zwui::divisionNames(), 1); }
    divisionAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::division, divisionBox);
    wobbleDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::wobbleDiv, wobbleDivBox);

    addAndMakeVisible(presetBox);
    refreshPresetBox();
    presetBox.onChange = [this] {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) processor.setCurrentProgram(idx);
    };

    setResizable(true, true);
    setResizeLimits(720, 450, 1440, 900);
    getConstrainer()->setFixedAspectRatio(1.6);
    setSize(880, 550);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
}

DriftAudioProcessorEditor::~DriftAudioProcessorEditor() { setLookAndFeel(nullptr); }

void DriftAudioProcessorEditor::refreshPresetBox()
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

void DriftAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit(0.0f, 0.1f, float((now - lastTick) * 0.001));
    lastTick = now;
    stage.tick(processor.motion, dt);
    lnf.pulse = processor.motion.pulse;

    if (processor.getCurrentProgram() != lastPreset)
    {
        lastPreset = processor.getCurrentProgram();
        presetBox.setSelectedId(lastPreset + 1, juce::dontSendNotification);
    }
    const bool sync = syncButton.getToggleState();
    divisionBox.setEnabled(sync); divisionBox.setAlpha(sync ? 1.0f : 0.4f);
    time.slider.setAlpha(sync ? 0.35f : 1.0f);
    const bool wsync = wobbleSyncButton.getToggleState();
    wobbleDivBox.setEnabled(wsync); wobbleDivBox.setAlpha(wsync ? 1.0f : 0.4f);
    wobbleRate.slider.setAlpha(wsync ? 0.35f : 1.0f);
    const bool on = powerButton.getToggleState();
    for (auto* k : { &time, &feedback, &wobble, &age, &space, &mix, &size, &decay, &width, &spread, &output, &wobbleRate })
        k->slider.setAlpha(on ? k->slider.getAlpha() : 0.3f);

    repaint();
}

void DriftAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getWidth() / 880.0f;
    stage.render(g, getLocalBounds(), lnf.accent, lnf.accent2, processor.motion, 42);
    zwui::drawHeader(g, r, s, "DRIFT", "LO-FI SPACE ECHO", lnf.accent, lnf.accentHi, lnf.accent2);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 84 * s, 836 * s, 262 * s), "ECHO", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 358 * s, 836 * s, 170 * s), "DEEP SPACE", lnf.accent2, lnf.accent, s);
    // Orbit visualizer in the header, reacting to the wet output
    zwui::drawOrbit(g, { 470 * s, 44 * s }, 26 * s, processor.motion, lnf.accent, lnf.accent2);
}

void DriftAudioProcessorEditor::resized()
{
    const float s = float(getWidth()) / 880.0f;
    auto S = [s](int v) { return juce::roundToInt(v * s); };

    powerButton.setBounds(S(400), S(22), S(34), S(34));
    presetBox.setBounds(S(520), S(24), S(250), S(30));
    freezeButton.setBounds(S(780), S(24), S(78), S(30));

    zwui::Knob* bigs[] = { &time, &feedback, &wobble, &age, &space, &mix };
    int x = S(44);
    for (auto* k : bigs) { k->layout(x, S(126), S(126), S(160), S(16)); x += S(134); }

    // Transport row: delay sync under TIME, wobble sync under WOBBLE
    syncButton.setBounds(S(52), S(300), S(58), S(26));
    divisionBox.setBounds(S(116), S(300), S(76), S(26));
    pingPongButton.setBounds(S(200), S(300), S(100), S(26));
    wobbleSyncButton.setBounds(S(330), S(300), S(58), S(26));
    wobbleDivBox.setBounds(S(394), S(300), S(84), S(26));

    zwui::Knob* smalls[] = { &size, &decay, &width, &spread, &wobbleRate, &output };
    x = S(50);
    for (auto* k : smalls) { k->layout(x, S(396), S(104), S(116), S(14)); x += S(132); }
}
