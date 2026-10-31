#include "PluginEditor.h"

PulseAudioProcessorEditor::PulseAudioProcessorEditor(PulseAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      presetBar(p.bank, { [] { return Presets::count(); }, [](int i) { return Presets::name(i); }, [](int i) { return Presets::category(i); },
                          [&p] { return p.getCurrentProgram(); }, [&p](int i) { p.setCurrentProgram(i); } }, lnf)
{
    setLookAndFeel(&lnf);
    auto& s = processor.apvts;
    depth.init   (*this, s, ParamIDs::depth,    "DEPTH",   true);
    time.init    (*this, s, ParamIDs::time,     "TIME",    true);
    shapeAtk.init(*this, s, ParamIDs::shapeAtk, "ATTACK",  true);
    shapeSus.init(*this, s, ParamIDs::shapeSus, "SUSTAIN", true);
    mix.init     (*this, s, ParamIDs::mix,      "MIX",     true);
    output.init  (*this, s, ParamIDs::output,   "OUTPUT",  true);
    input.init    (*this, s, ParamIDs::in,        "INPUT",   false);
    duck.init     (*this, s, ParamIDs::duck,      "DUCK",    false);
    scAmount.init (*this, s, ParamIDs::scAmount,  "SC AMT",  false);
    scRelease.init(*this, s, ParamIDs::scRelease, "SC REL",  false);
    lowX.init     (*this, s, ParamIDs::lowX,      "LOW X",   false);
    highX.init    (*this, s, ParamIDs::highX,     "HIGH X",  false);
    for (auto* b : { &autoButton, &subLockButton, &gritButton, &powerButton }) addAndMakeVisible(b);
    powerButton.getProperties().set("round", true);
    autoAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::autoMode, autoButton);
    subLockAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::subLock, subLockButton);
    gritAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::grit, gritButton);
    powerAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::power, powerButton);
    autoButton.setTooltip("Auto: thresholds follow a slow loudness average of each band, so DEPTH does the same thing on a quiet intro and a loud drop. Off = fixed thresholds around -18 dBFS (classic OTT behaviour).");
    subLockButton.setTooltip("Sub Lock: no dynamics below the LOW crossover - the sub stays exactly as you wrote it (band gain still applies)");
    gritButton.setTooltip("Grit: a gentle tape stage after the dynamics, so heavy compression adds warmth instead of flattening");
    powerButton.setTooltip("Power: clean 12 ms crossfade bypass");

    const char* bandNames[3] = { "LOW", "MID", "HIGH" };
    for (int b = 0; b < 3; ++b)
    {
        bandDepth[b].init(*this, s, ParamIDs::bandDepth(b).toRawUTF8(), "DEPTH", false);
        bandGain[b].init (*this, s, ParamIDs::bandGain(b).toRawUTF8(),  "GAIN",  false);
        bandAtk[b].init  (*this, s, ParamIDs::bandAtk(b).toRawUTF8(),   "ATK",   false);
        bandRel[b].init  (*this, s, ParamIDs::bandRel(b).toRawUTF8(),   "REL",   false);
        addAndMakeVisible(bandOn[b]); bandOn[b].getProperties().set("round", true);
        bandOn[b].setTooltip(juce::String(bandNames[b]) + " band dynamics on/off (off = passes clean)");
        bandOnAttach[b] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::bandOn(b), bandOn[b]);
        addAndMakeVisible(bandSolo[b]); bandSolo[b].setButtonText("S"); bandSolo[b].setTooltip("Solo this band");
        bandSoloAttach[b] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::bandSolo(b), bandSolo[b]);
    }
    for (int i = 0; i < 4; ++i)
    {
        nodeFreq[i].init (*this, s, ParamIDs::nodeFreq(i).toRawUTF8(),  "FREQ",  false, false);
        nodeQ[i].init    (*this, s, ParamIDs::nodeQ(i).toRawUTF8(),     "WIDTH", false, false);
        nodeThr[i].init  (*this, s, ParamIDs::nodeThr(i).toRawUTF8(),   "THR",   false, false);
        nodeRange[i].init(*this, s, ParamIDs::nodeRange(i).toRawUTF8(), "RANGE", false, false);
        addAndMakeVisible(nodeOn[i]); nodeOn[i].setButtonText(juce::String(i + 1));
        nodeOn[i].setTooltip("Dynamic EQ node " + juce::String(i + 1) + " on/off. Also drag it on the spectrum.");
        nodeOnAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::nodeOn(i), nodeOn[i]);
    }

    addAndMakeVisible(presetBar);
    addAndMakeVisible(modPanel);
    addAndMakeVisible(pad);
    addAndMakeVisible(glideBox); glideBox.addItemList(zwui::PadIDs::glideNames(), 1);
    glideBox.setTooltip("Glide: how long the pad takes to return home after you let go (synced)");
    glideAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, zwui::PadIDs::glide, glideBox);
    addAndMakeVisible(overlay);
    addAndMakeVisible(matchButton);
    matchButton.getProperties().set("chip", true);
    matchButton.setTooltip("Match: play the track for 6 seconds. Places the band splits around your sub and top end and sets OUTPUT so the result is as loud as the input - compare without the loudness lie.");
    matchButton.onClick = [this] { if (! learner.isActive()) { learner.start(6.0f); matchIn = matchOut = 0.0; matchFrames = 0; } };

    allKnobs = { &depth, &time, &shapeAtk, &shapeSus, &mix, &output, &input, &duck, &scAmount, &scRelease, &lowX, &highX };
    for (int b = 0; b < 3; ++b) { allKnobs.push_back(&bandDepth[b]); allKnobs.push_back(&bandGain[b]); allKnobs.push_back(&bandAtk[b]); allKnobs.push_back(&bandRel[b]); }
    for (int i = 0; i < 4; ++i) { allKnobs.push_back(&nodeFreq[i]); allKnobs.push_back(&nodeQ[i]); allKnobs.push_back(&nodeThr[i]); allKnobs.push_back(&nodeRange[i]); }
    installLocks();

    addAndMakeVisible(helpButton);
    helpButton.getProperties().set("chip", true); helpButton.getProperties().set("book", true);
    helpButton.setTooltip("Manual: what every knob does, quick start and recipes");
    helpButton.onClick = [this] { manual.show(); };
    addChildComponent(manual);
    manual.set("PULSE", "Multiband dynamics for bass music.", PulseManual::sections());
    zwui::applyTooltips(PulseManual::sections(), allKnobs);

    setResizable(true, true);
    setResizeLimits(704, 792, 1408, 1584);
    getConstrainer()->setFixedAspectRatio(880.0 / 990.0);
    setSize(880, 990);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
}

PulseAudioProcessorEditor::~PulseAudioProcessorEditor() { setLookAndFeel(nullptr); }

void PulseAudioProcessorEditor::installLocks()
{
    for (auto* k : allKnobs)
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

void PulseAudioProcessorEditor::syncModRings()
{
    const auto& targets = pulseModTargets();
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

void PulseAudioProcessorEditor::applyMatch()
{
    const auto r = learner.result;
    auto set = [this](const char* id, float plain) { if (auto* p = processor.apvts.getParameter(id)) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(plain)); p->endChangeGesture(); } };
    if (r.valid)
    {
        set(ParamIDs::lowX,  juce::jlimit(40.0f, 400.0f, r.subHz * 2.2f));
        set(ParamIDs::highX, juce::jlimit(1000.0f, 12000.0f, r.rollOffHz * 0.45f));
    }
    if (matchFrames > 10 && matchIn > 1e-6 && matchOut > 1e-6)
    {
        const float deltaDb = float(10.0 * std::log10(matchIn / matchOut));
        set(ParamIDs::output, juce::jlimit(-24.0f, 12.0f, float(output.slider.getValue()) + deltaDb));
    }
}

void PulseAudioProcessorEditor::timerCallback()
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
    if (learner.isActive()) { const float i = processor.inRms.load(), o = processor.outRms.load(); matchIn += i * i; matchOut += o * o; ++matchFrames; }
    if (learner.feed(analyserIn.levels, dt)) applyMatch();
    matchButton.setButtonText(learner.isActive() ? "MATCHING " + juce::String(int(learner.progress() * 100)) + "%" : "MATCH");
    pad.setHome(float(depth.slider.getValue()), float(duck.slider.getValue()));
    pad.tick();
    syncModRings();
    const bool on = powerButton.getToggleState();
    for (auto* k : allKnobs) k->slider.setAlpha(on ? 1.0f : 0.3f);
    for (int b = 0; b < 3; ++b)
    {
        const float a = bandOn[b].getToggleState() ? 1.0f : 0.3f;
        for (auto* k : { &bandDepth[b], &bandAtk[b], &bandRel[b] }) k->slider.setAlpha(juce::jmin(k->slider.getAlpha(), a));
    }
    bandDepth[0].slider.setAlpha(subLockButton.getToggleState() ? 0.35f : bandDepth[0].slider.getAlpha());
    for (int i = 0; i < 4; ++i) { const float a = nodeOn[i].getToggleState() ? 1.0f : 0.35f; for (auto* k : { &nodeFreq[i], &nodeQ[i], &nodeThr[i], &nodeRange[i] }) k->slider.setAlpha(juce::jmin(k->slider.getAlpha(), a)); }
    const bool sc = processor.sidechainPresent.load();
    scAmount.slider.setAlpha(sc ? scAmount.slider.getAlpha() : 0.4f); scRelease.slider.setAlpha(sc ? scRelease.slider.getAlpha() : 0.4f);
    overlay.repaint();
    repaint();
}

void PulseAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getWidth() / 880.0f;
    stage.render(g, getLocalBounds(), lnf.accent, lnf.accent2, processor.motion, 2024);
    zwui::drawHeader(g, r, s, "PULSE", "MULTIBAND DYNAMICS", lnf.accent, lnf.accentHi, lnf.accent2);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 84 * s, 836 * s, 300 * s), "DYNAMICS", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 396 * s, 836 * s, 230 * s), "BANDS  +  DYNAMIC EQ", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 638 * s, 836 * s, 158 * s), "FIELD", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 808 * s, 836 * s, 170 * s), "MOTION", lnf.accent2, lnf.accent, s);

    g.setFont(juce::Font(juce::FontOptions(8.5f * s, juce::Font::bold)));
    g.setColour(zwui::Palette::dim);
    g.drawText("DUCK  /  SIDECHAIN" + juce::String(processor.sidechainPresent.load() ? "  (SC IN)" : "  (NO SC INPUT)"), juce::Rectangle<float>(378 * s, 374 * s, 260 * s, 10 * s), juce::Justification::centredLeft);
    g.drawText("BAND SPLITS", juce::Rectangle<float>(658 * s, 374 * s, 160 * s, 10 * s), juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawVerticalLine(int(366 * s), 300 * s, 372 * s); g.drawVerticalLine(int(646 * s), 300 * s, 372 * s);

    const char* names[3] = { "LOW", "MID", "HIGH" };
    for (int b = 0; b < 3; ++b)
    {
        const float x0 = (36 + b * 272) * s;
        g.setFont(juce::Font(juce::FontOptions(9.5f * s, juce::Font::bold))); g.setColour(lnf.accentHi);
        g.drawText(names[b], juce::Rectangle<float>(x0 + 68 * s, 422 * s, 40 * s, 14 * s), juce::Justification::centredLeft);
        const float gr = processor.bandGr[size_t(b)].load();
        // GR meter: a bar that fills left (reduction) or right (upward) of a centre mark
        const auto m = juce::Rectangle<float>(x0 + 110 * s, 424 * s, 142 * s, 10 * s);
        g.setColour(zwui::Palette::bg.withAlpha(0.7f)); g.fillRoundedRectangle(m, 5 * s);
        const float half = m.getWidth() * 0.5f, len = juce::jlimit(-1.0f, 1.0f, gr / 18.0f) * half;
        g.setColour(gr < 0 ? zwui::Palette::magenta.withAlpha(0.85f) : lnf.accent2.withAlpha(0.85f));
        g.fillRoundedRectangle(len < 0 ? juce::Rectangle<float>(m.getCentreX() + len, m.getY() + 2, -len, m.getHeight() - 4) : juce::Rectangle<float>(m.getCentreX(), m.getY() + 2, len, m.getHeight() - 4), 3 * s);
        g.setColour(juce::Colours::white.withAlpha(0.3f)); g.drawVerticalLine(int(m.getCentreX()), m.getY(), m.getBottom());
        g.setFont(juce::Font(juce::FontOptions(8.0f * s, juce::Font::bold))); g.setColour(zwui::Palette::dim);
        g.drawText(juce::String(gr > 0 ? "+" : "") + juce::String(gr, 1) + " dB", juce::Rectangle<float>(x0 + 110 * s, 436 * s, 142 * s, 10 * s), juce::Justification::centredRight);
        if (b > 0) { g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawVerticalLine(int(x0 - 8 * s), 420 * s, 540 * s); }
    }
    // node row label + per-node live gain readout
    g.setFont(juce::Font(juce::FontOptions(8.5f * s, juce::Font::bold))); g.setColour(lnf.accentHi);
    g.drawText("DYNAMIC EQ  -  drag the numbered nodes on the spectrum", juce::Rectangle<float>(36 * s, 546 * s, 500 * s, 12 * s), juce::Justification::centredLeft);
    for (int i = 0; i < 4; ++i)
    {
        const float x0 = (36 + i * 206) * s; const float gdb = processor.nodeGain[size_t(i)].load();
        g.setColour(std::abs(gdb) > 0.05f ? (gdb < 0 ? zwui::Palette::magenta : lnf.accent2) : zwui::Palette::dim);
        g.drawText(juce::String(gdb > 0 ? "+" : "") + juce::String(gdb, 1), juce::Rectangle<float>(x0 + 150 * s, 608 * s, 48 * s, 10 * s), juce::Justification::centredRight);
    }

    scope.draw(g, juce::Rectangle<float>(40 * s, 656 * s, 128 * s, 128 * s), lnf.accent, lnf.accent2, processor.motion.pulse);
    zwui::EqCurve eq { 20.0f, 20000.0f, 0, 0, 0.0f };
    zwui::drawEqField(g, juce::Rectangle<float>(320 * s, 660 * s, 278 * s, 120 * s), analyser, &analyserIn, eq, lnf.accent, lnf.accent2, processor.motion.pulse);
    // right of the spectrum: transient + duck meters and the density readout
    {
        auto meter = [&](float x, float v, const char* label, juce::Colour col)
        {
            const auto m = juce::Rectangle<float>(x, 672 * s, 18 * s, 96 * s);
            g.setColour(zwui::Palette::bg.withAlpha(0.7f)); g.fillRoundedRectangle(m, 6 * s);
            const float h = juce::jlimit(0.0f, 1.0f, v) * (m.getHeight() - 4 * s);
            g.setGradientFill(juce::ColourGradient(col, m.getX(), m.getBottom(), col.withAlpha(0.3f), m.getX(), m.getY(), false));
            g.fillRoundedRectangle(juce::Rectangle<float>(m.getX() + 2 * s, m.getBottom() - 2 * s - h, m.getWidth() - 4 * s, h), 4 * s);
            g.setColour(lnf.accent.withAlpha(0.5f)); g.drawRoundedRectangle(m, 6 * s, 1.0f);
            g.setFont(juce::Font(juce::FontOptions(7.5f * s, juce::Font::bold))); g.setColour(zwui::Palette::dim);
            g.drawText(label, juce::Rectangle<float>(x - 14 * s, 770 * s, 46 * s, 10 * s), juce::Justification::centred);
        };
        meter(624 * s, processor.transientVis.load(), "HITS", lnf.accent2);
        meter(668 * s, processor.duckVis.load(), "DUCK", zwui::Palette::magenta);
        const float in = processor.inRms.load(), out = processor.outRms.load();
        const float delta = (in > 1e-5f && out > 1e-5f) ? 20.0f * std::log10(out / in) : 0.0f;
        g.setFont(juce::Font(juce::FontOptions(9.0f * s, juce::Font::bold))); g.setColour(lnf.accentHi);
        g.drawText("LEVEL", juce::Rectangle<float>(712 * s, 672 * s, 130 * s, 12 * s), juce::Justification::centredLeft);
        g.setFont(juce::Font(juce::FontOptions(20.0f * s, juce::Font::bold))); g.setColour(std::abs(delta) < 1.0f ? lnf.accent2 : zwui::Palette::text);
        g.drawText((delta > 0 ? "+" : "") + juce::String(delta, 1) + " dB", juce::Rectangle<float>(712 * s, 686 * s, 130 * s, 26 * s), juce::Justification::centredLeft);
    }
    zwui::drawOrbit(g, { 392 * s, 42 * s }, 24 * s, processor.motion, lnf.accent, lnf.accent2);
}

void PulseAudioProcessorEditor::resized()
{
    const float s = float(getWidth()) / 880.0f;
    auto S = [s](int v) { return juce::roundToInt(v * s); };

    presetBar.setBounds(0, 0, getWidth(), S(60));
    presetBar.layoutScaled(s, 424);
    helpButton.setBounds(S(776), S(61), S(82), S(18));
    manual.setBounds(getLocalBounds());
    powerButton.setBounds(S(824), S(20), S(34), S(34));

    zwui::Knob* bigs[] = { &depth, &time, &shapeAtk, &shapeSus, &mix, &output };
    int x = S(44);
    for (auto* k : bigs) { k->layout(x, S(126), S(126), S(160), S(16)); x += S(134); }
    autoButton.setBounds(S(52), S(322), S(66), S(26));
    subLockButton.setBounds(S(124), S(322), S(92), S(26));
    gritButton.setBounds(S(222), S(322), S(60), S(26));
    input.layout    (S(292), S(306), S(66), S(66), S(11));
    duck.layout     (S(378), S(306), S(66), S(66), S(11));
    scAmount.layout (S(458), S(306), S(66), S(66), S(11));
    scRelease.layout(S(538), S(306), S(66), S(66), S(11));
    lowX.layout     (S(660), S(306), S(66), S(66), S(11));
    highX.layout    (S(740), S(306), S(66), S(66), S(11));

    for (int b = 0; b < 3; ++b)
    {
        const int x0 = S(36 + b * 272);
        bandOn[b].setBounds(x0, S(420), S(24), S(24));
        bandSolo[b].setBounds(x0 + S(30), S(422), S(28), S(20));
        bandDepth[b].layout(x0,          S(464), S(60), S(72), S(12));
        bandGain[b].layout (x0 + S(64),  S(464), S(60), S(72), S(12));
        bandAtk[b].layout  (x0 + S(128), S(464), S(60), S(72), S(12));
        bandRel[b].layout  (x0 + S(192), S(464), S(60), S(72), S(12));
    }
    for (int i = 0; i < 4; ++i)
    {
        const int x0 = S(36 + i * 206);
        nodeOn[i].setBounds(x0, S(572), S(22), S(22));
        nodeFreq[i].layout (x0 + S(26),  S(570), S(40), S(38), S(10));
        nodeQ[i].layout    (x0 + S(68),  S(570), S(40), S(38), S(10));
        nodeThr[i].layout  (x0 + S(110), S(570), S(40), S(38), S(10));
        nodeRange[i].layout(x0 + S(152), S(570), S(40), S(38), S(10));
    }

    pad.setBounds(S(184), S(658), S(122), S(106));
    glideBox.setBounds(S(184), S(768), S(122), S(18));
    overlay.setBounds(S(320), S(660), S(278), S(120));
    matchButton.setBounds(S(744), S(644), S(100), S(17));

    modPanel.setBounds(S(22), S(808), S(836), S(170));
    modPanel.layoutScaled(s);
}
