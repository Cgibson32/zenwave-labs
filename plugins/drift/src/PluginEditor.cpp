#include "PluginEditor.h"

DriftAudioProcessorEditor::DriftAudioProcessorEditor(DriftAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      presetBar(p.bank, { [] { return Presets::count(); }, [](int i) { return Presets::name(i); }, [](int i) { return Presets::category(i); },
                          [&p] { return p.getCurrentProgram(); }, [&p](int i) { p.setCurrentProgram(i); } }, lnf)
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
    color.init     (*this, processor.apvts, ParamIDs::color,      "COLOR",  false);
    lowCut.init    (*this, processor.apvts, ParamIDs::lowCut,     "LOW CUT",  false);
    highCut.init   (*this, processor.apvts, ParamIDs::highCut,    "HIGH CUT", false);
    tilt.init      (*this, processor.apvts, ParamIDs::tilt,       "TILT",     false);
    for (auto* box : { &lowSlopeBox, &highSlopeBox }) { addAndMakeVisible(box); box->addItemList({ "6 dB", "12 dB", "24 dB", "48 dB" }, 1); }
    lowSlopeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::lowSlope, lowSlopeBox);
    highSlopeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::highSlope, highSlopeBox);

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

    addAndMakeVisible(presetBar);
    addAndMakeVisible(modPanel);
    addAndMakeVisible(pad);
    addAndMakeVisible(glideBox); glideBox.addItemList(zwui::PadIDs::glideNames(), 1);
    glideBox.setTooltip("Glide: how long the pad takes to return home after you let go (synced)");
    glideAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, zwui::PadIDs::glide, glideBox);
    allKnobs = { &time, &feedback, &wobble, &age, &space, &mix, &size, &decay, &width, &spread, &output, &wobbleRate, &color, &lowCut, &highCut, &tilt };
    installLocks();

    // Built-in manual + tooltips
    addAndMakeVisible(helpButton);
    helpButton.getProperties().set("chip", true);
    helpButton.setTooltip("Manual: what every knob does, quick start and recipes");
    helpButton.onClick = [this] { manual.show(); };
    addChildComponent(manual);
    manual.set("DRIFT", "Lo-fi space echo for bass music.", DriftManual::sections());
    zwui::applyTooltips(DriftManual::sections(), allKnobs);
    powerButton.setTooltip("Power: clean 12 ms crossfade bypass");
    syncButton.setTooltip("Sync the delay time to host tempo (uses the division next to it)");
    divisionBox.setTooltip("Delay division when SYNC is on. Dotted (.) and triplet (T) values included.");
    pingPongButton.setTooltip("Ping-pong: repeats alternate left / right");
    wobbleSyncButton.setTooltip("Lock the slow wow to tempo so the pitch dip lands on the beat");
    wobbleDivBox.setTooltip("Wobble division: one wow cycle per division");
    freezeButton.setTooltip("Freeze: hold the current tail forever. Automate it for drops and transitions.");

    setResizable(true, true);
    setResizeLimits(704, 712, 1408, 1424);
    getConstrainer()->setFixedAspectRatio(880.0 / 890.0);
    setSize(880, 890);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
}

DriftAudioProcessorEditor::~DriftAudioProcessorEditor() { setLookAndFeel(nullptr); }

void DriftAudioProcessorEditor::installLocks()
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

void DriftAudioProcessorEditor::syncModRings()
{
    const auto& targets = driftModTargets();
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

void DriftAudioProcessorEditor::timerCallback()
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
    pad.setHome(processor.padXForTime(float(time.slider.getValue())), processor.padYForFeedback(float(feedback.slider.getValue())));
    pad.setColumns(syncButton.getToggleState() ? 9 : 0);
    pad.tick();
    syncModRings();
    const bool sync = syncButton.getToggleState();
    divisionBox.setEnabled(sync); divisionBox.setAlpha(sync ? 1.0f : 0.4f);
    time.slider.setAlpha(sync ? 0.35f : 1.0f);
    const bool wsync = wobbleSyncButton.getToggleState();
    wobbleDivBox.setEnabled(wsync); wobbleDivBox.setAlpha(wsync ? 1.0f : 0.4f);
    wobbleRate.slider.setAlpha(wsync ? 0.35f : 1.0f);
    const bool on = powerButton.getToggleState();
    for (auto* k : { &time, &feedback, &wobble, &age, &space, &mix, &size, &decay, &width, &spread, &output, &wobbleRate, &color })
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
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 358 * s, 836 * s, 160 * s), "DEEP SPACE", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 530 * s, 836 * s, 158 * s), "FIELD", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 700 * s, 836 * s, 170 * s), "MOTION", lnf.accent2, lnf.accent, s);
    scope.draw(g, juce::Rectangle<float>(40 * s, 548 * s, 128 * s, 128 * s), lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::EqCurve eq { lowCut.slider.getValue() > 0 ? float(lowCut.slider.getValue()) : 20.0f, float(highCut.slider.getValue()),
                       juce::jmax(0, lowSlopeBox.getSelectedId() - 1), juce::jmax(0, highSlopeBox.getSelectedId() - 1), float(tilt.slider.getValue()) };
    zwui::drawEqField(g, juce::Rectangle<float>(320 * s, 552 * s, 278 * s, 120 * s), analyser, &analyserIn, eq, lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::drawOrbit(g, { 392 * s, 42 * s }, 24 * s, processor.motion, lnf.accent, lnf.accent2);
}

void DriftAudioProcessorEditor::resized()
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

    zwui::Knob* bigs[] = { &time, &feedback, &wobble, &age, &space, &mix };
    int x = S(44);
    for (auto* k : bigs) { k->layout(x, S(126), S(126), S(160), S(16)); x += S(134); }

    // Transport row: delay sync under TIME, wobble sync under WOBBLE
    syncButton.setBounds(S(52), S(300), S(58), S(26));
    divisionBox.setBounds(S(116), S(300), S(76), S(26));
    pingPongButton.setBounds(S(200), S(300), S(100), S(26));
    wobbleSyncButton.setBounds(S(330), S(300), S(58), S(26));
    wobbleDivBox.setBounds(S(394), S(300), S(84), S(26));

    zwui::Knob* smalls[] = { &color, &size, &decay, &width, &spread, &wobbleRate, &output };
    x = S(40);
    for (auto* k : smalls) { k->layout(x, S(392), S(100), S(110), S(14)); x += S(115); }

    // Tone: field display on the left, cuts + tilt on the right
    pad.setBounds(S(184), S(550), S(122), S(106));
    glideBox.setBounds(S(184), S(660), S(122), S(18));
    zwui::Knob* tone[] = { &lowCut, &highCut, &tilt };
    x = S(610);
    for (auto* k : tone) { k->layout(x, S(572), S(80), S(84), S(12)); x += S(84); }
    lowSlopeBox.setBounds(S(614), S(662), S(72), S(18));
    highSlopeBox.setBounds(S(698), S(662), S(72), S(18));
}
