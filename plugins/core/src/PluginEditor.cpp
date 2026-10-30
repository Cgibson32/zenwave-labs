#include "PluginEditor.h"

CoreAudioProcessorEditor::CoreAudioProcessorEditor(CoreAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      presetBar(p.bank, { [] { return Presets::count(); }, [](int i) { return Presets::name(i); }, [](int i) { return Presets::category(i); },
                          [&p] { return p.getCurrentProgram(); }, [&p](int i) { p.setCurrentProgram(i); } }, lnf)
{
    setLookAndFeel(&lnf);

    drive.init (*this, processor.apvts, ParamIDs::drive,  "DRIVE",  true);
    heat.init  (*this, processor.apvts, ParamIDs::heat,   "HEAT",   true);
    focus.init (*this, processor.apvts, ParamIDs::focus,  "FOCUS",  true);
    width.init (*this, processor.apvts, ParamIDs::width,  "WIDTH",  true);
    mix.init   (*this, processor.apvts, ParamIDs::mix,    "MIX",    true);
    output.init(*this, processor.apvts, ParamIDs::output, "OUTPUT", true);
    const char* bandNames[3] = { "LOW", "MID", "HIGH" };
    for (int b = 0; b < 3; ++b)
    {
        bandDrive[b].init(*this, processor.apvts, ParamIDs::bandDrive(b).toRawUTF8(), "DRIVE", false);
        bandMix[b].init  (*this, processor.apvts, ParamIDs::bandMix(b).toRawUTF8(),   "MIX",   false);
        bandGain[b].init (*this, processor.apvts, ParamIDs::bandGain(b).toRawUTF8(),  "GAIN",  false);
        addAndMakeVisible(bandChar[b]); bandChar[b].addItemList({ "Tape", "Tube", "Fold", "Rectify", "Crush" }, 1);
        bandChar[b].setTooltip(juce::String(bandNames[b]) + " band character: Tape (smooth), Tube (warm, even harmonics), Fold (wavefolder, synthy), Rectify (fuzz, aggressive), Crush (bit + rate reduction)");
        bandCharAttach[b] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::bandChar(b), bandChar[b]);
    }
    lowCut.init (*this, processor.apvts, ParamIDs::lowCut,  "LOW CUT",  false);
    highCut.init(*this, processor.apvts, ParamIDs::highCut, "HIGH CUT", false);
    tilt.init   (*this, processor.apvts, ParamIDs::tilt,    "TILT",     false);
    for (auto* box : { &lowSlopeBox, &highSlopeBox }) { addAndMakeVisible(box); box->addItemList({ "6 dB", "12 dB", "24 dB", "48 dB" }, 1); }
    lowSlopeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::lowSlope, lowSlopeBox);
    highSlopeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::highSlope, highSlopeBox);

    for (auto* b : { &subLockButton, &powerButton }) addAndMakeVisible(b);
    powerButton.getProperties().set("round", true);
    subLockAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::subLock, subLockButton);
    powerAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::power, powerButton);
    subLockButton.setTooltip("Sub Lock: everything below the LOW crossover stays clean and mono, however hard you drive the rest");
    powerButton.setTooltip("Power: clean 12 ms crossfade bypass");

    addAndMakeVisible(presetBar);
    addAndMakeVisible(modPanel);
    addAndMakeVisible(pad);
    addAndMakeVisible(glideBox); glideBox.addItemList(zwui::PadIDs::glideNames(), 1);
    glideBox.setTooltip("Glide: how long the pad takes to return home after you let go (synced)");
    glideAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, zwui::PadIDs::glide, glideBox);
    addAndMakeVisible(dividers);
    addAndMakeVisible(learnButton);
    learnButton.getProperties().set("chip", true);
    learnButton.setTooltip("Learn: play the track for 6 seconds. Places the LOW crossover above your sub and the HIGH crossover under your top end so the bands fit this track.");
    learnButton.onClick = [this] { if (! learner.isActive()) learner.start(6.0f); };

    allKnobs = { &drive, &heat, &focus, &width, &mix, &output, &lowCut, &highCut, &tilt };
    for (int b = 0; b < 3; ++b) { allKnobs.push_back(&bandDrive[b]); allKnobs.push_back(&bandMix[b]); allKnobs.push_back(&bandGain[b]); }
    installLocks();

    addAndMakeVisible(helpButton);
    helpButton.getProperties().set("chip", true); helpButton.getProperties().set("book", true);
    helpButton.setTooltip("Manual: what every knob does, quick start and recipes");
    helpButton.onClick = [this] { manual.show(); };
    addChildComponent(manual);
    manual.set("CORE", "Multiband heat for bass music.", CoreManual::sections());
    zwui::applyTooltips(CoreManual::sections(), allKnobs);

    setResizable(true, true);
    setResizeLimits(704, 712, 1408, 1424);
    getConstrainer()->setFixedAspectRatio(880.0 / 890.0);
    setSize(880, 890);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
}

CoreAudioProcessorEditor::~CoreAudioProcessorEditor() { setLookAndFeel(nullptr); }

void CoreAudioProcessorEditor::installLocks()
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

void CoreAudioProcessorEditor::syncModRings()
{
    const auto& targets = coreModTargets();
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

void CoreAudioProcessorEditor::applyLearn()
{
    const auto r = learner.result;
    if (! r.valid) return;
    auto set = [this](const char* id, float plain) { if (auto* p = processor.apvts.getParameter(id)) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(plain)); p->endChangeGesture(); } };
    set(ParamIDs::lowX,  juce::jlimit(40.0f, 400.0f, r.subHz * 2.2f));            // sub + first harmonic stay in the locked band
    set(ParamIDs::highX, juce::jlimit(1000.0f, 12000.0f, r.rollOffHz * 0.45f));  // high band = the top octave of what's actually there
}

void CoreAudioProcessorEditor::timerCallback()
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
    if (learner.feed(analyserIn.levels, dt)) applyLearn();
    learnButton.setButtonText(learner.isActive() ? "LEARNING " + juce::String(int(learner.progress() * 100)) + "%" : "LEARN");
    pad.setHome(float(drive.slider.getValue()), float(mix.slider.getValue()));
    pad.tick();
    syncModRings();
    const bool on = powerButton.getToggleState();
    for (auto* k : allKnobs) k->slider.setAlpha(on ? 1.0f : 0.3f);
    bandDrive[0].slider.setAlpha(subLockButton.getToggleState() ? 0.35f : (on ? 1.0f : 0.3f));
    bandChar[0].setAlpha(subLockButton.getToggleState() ? 0.4f : 1.0f);
    dividers.repaint();
    repaint();
}

void CoreAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getWidth() / 880.0f;
    stage.render(g, getLocalBounds(), lnf.accent, lnf.accent2, processor.motion, 99);
    zwui::drawHeader(g, r, s, "CORE", "MULTIBAND HEAT", lnf.accent, lnf.accentHi, lnf.accent2);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 84 * s, 836 * s, 262 * s), "HEAT", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 358 * s, 836 * s, 160 * s), "BANDS", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 530 * s, 836 * s, 158 * s), "FIELD", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 700 * s, 836 * s, 170 * s), "MOTION", lnf.accent2, lnf.accent, s);

    // band column titles + auto-gain readout + column separators
    const char* names[3] = { "LOW", "MID", "HIGH" };
    g.setFont(juce::Font(juce::FontOptions(9.5f * s, juce::Font::bold)));
    for (int b = 0; b < 3; ++b)
    {
        const float x0 = (36 + b * 272) * s;
        g.setColour(lnf.accentHi);
        g.drawText(names[b], juce::Rectangle<float>(x0 + 118 * s, 385 * s, 60 * s, 14 * s), juce::Justification::centredLeft);
        const float ag = processor.bandGainDb[size_t(b)].load();
        g.setColour(zwui::Palette::dim);
        g.drawText("AUTO " + juce::String(ag > 0 ? "+" : "") + juce::String(ag, 1) + " dB", juce::Rectangle<float>(x0 + 150 * s, 385 * s, 100 * s, 14 * s), juce::Justification::centredLeft);
        if (b > 0) { g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawVerticalLine(int(x0 - 8 * s), 380 * s, 512 * s); }
    }

    scope.draw(g, juce::Rectangle<float>(40 * s, 548 * s, 128 * s, 128 * s), lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::EqCurve eq { float(lowCut.slider.getValue()), float(highCut.slider.getValue()),
                       juce::jmax(0, lowSlopeBox.getSelectedId() - 1), juce::jmax(0, highSlopeBox.getSelectedId() - 1), float(tilt.slider.getValue()) };
    zwui::drawEqField(g, juce::Rectangle<float>(320 * s, 552 * s, 278 * s, 120 * s), analyser, &analyserIn, eq, lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::drawOrbit(g, { 392 * s, 42 * s }, 24 * s, processor.motion, lnf.accent, lnf.accent2);
}

void CoreAudioProcessorEditor::resized()
{
    const float s = float(getWidth()) / 880.0f;
    auto S = [s](int v) { return juce::roundToInt(v * s); };

    presetBar.setBounds(0, 0, getWidth(), S(60));
    presetBar.layoutScaled(s, 424);
    helpButton.setBounds(S(776), S(61), S(82), S(18));
    manual.setBounds(getLocalBounds());
    powerButton.setBounds(S(824), S(20), S(34), S(34));

    zwui::Knob* bigs[] = { &drive, &heat, &focus, &width, &mix, &output };
    int x = S(44);
    for (auto* k : bigs) { k->layout(x, S(126), S(126), S(160), S(16)); x += S(134); }
    subLockButton.setBounds(S(52), S(300), S(96), S(26));

    for (int b = 0; b < 3; ++b)
    {
        const int x0 = S(36 + b * 272);
        bandChar[b].setBounds(x0, S(382), S(110), S(20));
        bandDrive[b].layout(x0,          S(422), S(80), S(88), S(12));
        bandMix[b].layout  (x0 + S(86),  S(422), S(80), S(88), S(12));
        bandGain[b].layout (x0 + S(172), S(422), S(80), S(88), S(12));
    }

    pad.setBounds(S(184), S(550), S(122), S(106));
    glideBox.setBounds(S(184), S(660), S(122), S(18));
    dividers.setBounds(S(320), S(552), S(278), S(120));
    learnButton.setBounds(S(744), S(536), S(100), S(17));
    zwui::Knob* tone[] = { &lowCut, &highCut, &tilt };
    x = S(610);
    for (auto* k : tone) { k->layout(x, S(572), S(80), S(84), S(12)); x += S(84); }
    lowSlopeBox.setBounds(S(614), S(662), S(72), S(18));
    highSlopeBox.setBounds(S(698), S(662), S(72), S(18));

    modPanel.setBounds(S(22), S(700), S(836), S(170));
    modPanel.layoutScaled(s);
}
