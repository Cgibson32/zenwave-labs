#include "PluginEditor.h"

UmbraAudioProcessorEditor::UmbraAudioProcessorEditor(UmbraAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      presetBar(p.bank, { [] { return Presets::count(); }, [](int i) { return Presets::name(i); }, [](int i) { return Presets::category(i); },
                          [&p] { return p.getCurrentProgram(); }, [&p](int i) { p.setCurrentProgram(i); } }, lnf)
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
    bloom.init     (*this, processor.apvts, ParamIDs::bloom,      "BLOOM",       false);
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

    addAndMakeVisible(presetBar);
    addAndMakeVisible(modPanel);
    allKnobs = { &preDelay, &size, &decay, &dark, &mix, &density, &motion, &gravity, &early, &duck, &width, &output, &motionRate, &bloom, &lowCut, &highCut, &tilt };
    installLocks();

    // Built-in manual + tooltips
    addAndMakeVisible(helpButton);
    helpButton.getProperties().set("chip", true);
    helpButton.setTooltip("Manual: what every knob does, quick start and recipes");
    helpButton.onClick = [this] { manual.show(); };
    addChildComponent(manual);
    manual.set("UMBRA", "Dark reverb for bass music.", UmbraManual::sections());
    zwui::applyTooltips(UmbraManual::sections(), allKnobs);
    powerButton.setTooltip("Power: clean 12 ms crossfade bypass");
    motionSyncButton.setTooltip("Lock the internal tail modulation to tempo: one slow swirl per division");
    motionDivBox.setTooltip("Motion division when SYNC is on");
    freezeButton.setTooltip("Freeze: hold the current tail forever. Automate it for drops and transitions.");

    setResizable(true, true);
    setResizeLimits(704, 712, 1408, 1424);
    getConstrainer()->setFixedAspectRatio(880.0 / 890.0);
    setSize(880, 890);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
}

UmbraAudioProcessorEditor::~UmbraAudioProcessorEditor() { setLookAndFeel(nullptr); }

void UmbraAudioProcessorEditor::installLocks()
{
    for (auto* k : allKnobs)
    {
        k->slider.onContextMenu = [this, k]
        {
            juce::PopupMenu m;
            m.addItem(1, processor.bank.isLocked(k->paramId) ? "Unlock for Warp" : "Lock for Warp");
            m.addItem(2, "Reset to default");
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(k->slider), [this, k](int r)
            {
                if (r == 1) processor.bank.toggleLock(k->paramId);
                if (r == 2) if (auto* rp = processor.apvts.getParameter(k->paramId)) { rp->beginChangeGesture(); rp->setValueNotifyingHost(rp->getDefaultValue()); rp->endChangeGesture(); }
            });
        };
    }
}

void UmbraAudioProcessorEditor::syncModRings()
{
    const auto& targets = umbraModTargets();
    for (auto* k : allKnobs)
    {
        k->setLocked(processor.bank.isLocked(k->paramId));
        int ti = -1;
        for (int i = 0; i < int(targets.size()); ++i) if (k->paramId == targets[size_t(i)].id) { ti = i; break; }
        const float pos = ti >= 0 ? processor.mod.modPos[size_t(ti)].load() : -1.0f;
        if (pos >= 0.0f) { const auto sp = processor.mod.span(ti); k->setMod(sp.first, sp.second, pos); }
        else k->clearMod();
    }
}

void UmbraAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit(0.0f, 0.1f, float((now - lastTick) * 0.001));
    lastTick = now;
    stage.tick(processor.motion, dt);
    lnf.pulse = processor.motion.pulse;
    analyser.update(processor.spectrum, processor.currentSampleRate);
    analyserIn.update(processor.spectrumIn, processor.currentSampleRate);
    scope.update(processor.stereo);
    presetBar.sync();
    modPanel.tick();
    syncModRings();
    const bool msync = motionSyncButton.getToggleState();
    motionDivBox.setEnabled(msync); motionDivBox.setAlpha(msync ? 1.0f : 0.4f);
    motionRate.slider.setAlpha(msync ? 0.35f : 1.0f);
    const bool on = powerButton.getToggleState();
    for (auto* k : { &preDelay, &size, &decay, &dark, &mix, &density, &motion, &gravity, &early, &duck, &width, &output, &motionRate, &bloom })
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
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 700 * s, 836 * s, 170 * s), "MOTION", lnf.accent2, lnf.accent, s);
    scope.draw(g, juce::Rectangle<float>(40 * s, 548 * s, 128 * s, 128 * s), lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::EqCurve eq { float(lowCut.slider.getValue()), float(highCut.slider.getValue()),
                       juce::jmax(0, lowSlopeBox.getSelectedId() - 1), juce::jmax(0, highSlopeBox.getSelectedId() - 1), float(tilt.slider.getValue()) };
    zwui::drawEqField(g, juce::Rectangle<float>(186 * s, 552 * s, 360 * s, 120 * s), analyser, &analyserIn, eq, lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::drawOrbit(g, { 392 * s, 42 * s }, 24 * s, processor.motion, lnf.accent, lnf.accent2);
}

void UmbraAudioProcessorEditor::resized()
{
    const float s = float(getWidth()) / 880.0f;
    auto S = [s](int v) { return juce::roundToInt(v * s); };

    presetBar.setBounds(0, 0, getWidth(), S(60));
    helpButton.setBounds(S(776), S(61), S(82), S(18));
    manual.setBounds(getLocalBounds());
    presetBar.layoutScaled(s, 424);
    powerButton.setBounds(S(824), S(20), S(34), S(34));
    freezeButton.setBounds(S(752), S(300), S(84), S(26));
    modPanel.setBounds(S(22), S(700), S(836), S(170));
    modPanel.layoutScaled(s);

    zwui::Knob* bigs[] = { &preDelay, &size, &decay, &dark, &mix };
    int x = S(66);
    for (auto* k : bigs) { k->layout(x, S(120), S(140), S(160), S(16)); x += S(156); }

    motionSyncButton.setBounds(S(52), S(300), S(58), S(26));
    motionDivBox.setBounds(S(116), S(300), S(84), S(26));

    zwui::Knob* smalls[] = { &bloom, &density, &motion, &motionRate, &gravity, &early, &duck, &width, &output };
    x = S(32);
    for (auto* k : smalls) { k->layout(x, S(392), S(88), S(110), S(14)); x += S(92); }

    zwui::Knob* tone[] = { &lowCut, &highCut, &tilt };
    x = S(566);
    for (auto* k : tone) { k->layout(x, S(572), S(90), S(84), S(12)); x += S(98); }
    lowSlopeBox.setBounds(S(574), S(662), S(74), S(18));
    highSlopeBox.setBounds(S(672), S(662), S(74), S(18));
}
