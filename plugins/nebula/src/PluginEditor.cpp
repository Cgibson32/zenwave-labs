#include "PluginEditor.h"

static constexpr float kW = 1040.0f, kH = 980.0f;

NebulaAudioProcessorEditor::NebulaAudioProcessorEditor(NebulaAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      presetBar(p.bank, { [] { return Presets::count(); }, [](int i) { return Presets::name(i); }, [](int i) { return Presets::category(i); },
                          [&p] { return p.getCurrentProgram(); }, [&p](int i) { p.setCurrentProgram(i); } }, lnf)
{
    setLookAndFeel(&lnf);
    auto& s = processor.apvts;
    addAndMakeVisible(planet);
    for (int o = 0; o < 2; ++o)
    {
        auto& u = osc[o];
        addAndMakeVisible(u.table); u.table.addItemList(nebula::tableNames(), 1);
        u.table.setTooltip("Wavetable: Glass (bells), Voice (formants), Metal (inharmonic), Grain (texture), Dune (saw-square-pulse), Halo (shimmering cluster), Ion (bright buzz), Void (dark sub)");
        u.tableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, ParamIDs::osc(o, "table"), u.table);
        addAndMakeVisible(u.on); u.on.getProperties().set("round", true); u.on.setTooltip("Oscillator on/off");
        u.onAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::osc(o, "on"), u.on);
        u.position.init(*this, s, ParamIDs::osc(o, "position").toRawUTF8(), "POSITION", true);
        u.level.init   (*this, s, ParamIDs::osc(o, "level").toRawUTF8(),    "LEVEL",  false, false);
        u.octave.init  (*this, s, ParamIDs::osc(o, "octave").toRawUTF8(),   "OCT",    false, false);
        u.semis.init   (*this, s, ParamIDs::osc(o, "semis").toRawUTF8(),    "SEMI",   false, false);
        u.fine.init    (*this, s, ParamIDs::osc(o, "fine").toRawUTF8(),     "FINE",   false, false);
        u.unison.init  (*this, s, ParamIDs::osc(o, "unison").toRawUTF8(),   "UNISON", false);
        u.spread.init  (*this, s, ParamIDs::osc(o, "spread").toRawUTF8(),   "SPREAD", false);
        u.width.init   (*this, s, ParamIDs::osc(o, "width").toRawUTF8(),    "WIDTH",  false);
    }
    cutoff.init(*this, s, ParamIDs::cutoff, "CUTOFF", true);
    res.init(*this, s, ParamIDs::res, "RES", false); envAmt.init(*this, s, ParamIDs::envAmt, "ENV", false);
    keyTrack.init(*this, s, ParamIDs::keyTrack, "KEY", false); drive.init(*this, s, ParamIDs::drive, "DRIVE", false);
    addAndMakeVisible(filterModeBox); filterModeBox.addItemList({ "Low", "High", "Band" }, 1); filterModeBox.setTooltip("Filter type");
    filterModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, ParamIDs::filterMode, filterModeBox);
    addAndMakeVisible(carveButton);
    carveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::carveOn, carveButton);
    carveButton.setTooltip("Carve on/off: the 8 hand-placed harmonic bands on the planet's halo are rendered into the wavetable itself");
    const char* ampIds[4] = { ParamIDs::ampA, ParamIDs::ampD, ParamIDs::ampS, ParamIDs::ampR }; const char* modIds[4] = { ParamIDs::modA, ParamIDs::modD, ParamIDs::modS, ParamIDs::modR };
    const char* adsr[4] = { "A", "D", "S", "R" };
    for (int i = 0; i < 4; ++i) { ampK[i].init(*this, s, ampIds[i], adsr[i], false, false); modK[i].init(*this, s, modIds[i], adsr[i], false, false); }
    spaceSize.init(*this, s, ParamIDs::spaceSize, "SIZE", false, false); spaceDecay.init(*this, s, ParamIDs::spaceDecay, "DECAY", false, false); spaceDark.init(*this, s, ParamIDs::spaceDark, "DARK", false, false);
    duck.init(*this, s, ParamIDs::duck, "DUCK", false, false); duckRel.init(*this, s, ParamIDs::duckRelease, "REL", false, false);
    glide.init(*this, s, ParamIDs::glide, "GLIDE", false, false); drift.init(*this, s, ParamIDs::drift, "DRIFT", false, false); volume.init(*this, s, ParamIDs::volume, "VOL", false, false);
    const char* macroIds[8] = { ParamIDs::mWarmth, ParamIDs::mAir, ParamIDs::mMovement, ParamIDs::mSpace, ParamIDs::mGrit, ParamIDs::mWidth, ParamIDs::mAttack, ParamIDs::mRelease };
    const char* macroNames[8] = { "WARMTH", "AIR", "MOVEMENT", "SPACE", "GRIT", "WIDTH", "ATTACK", "RELEASE" };
    for (int i = 0; i < 8; ++i) macro[i].init(*this, s, macroIds[i], macroNames[i], false);
    addAndMakeVisible(powerButton); powerButton.getProperties().set("round", true); powerButton.setTooltip("Power: smooth output mute");
    powerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::power, powerButton);

    addAndMakeVisible(presetBar);
    addAndMakeVisible(modPanel);
    addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(36, 96); keyboard.setKeyWidth(16.0f); keyboard.setScrollButtonsVisible(false);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, zwui::Palette::card.brighter(0.05f));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, zwui::Palette::bg);
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, zwui::Palette::cardEdge);
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, lnf.accent.withAlpha(0.7f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, lnf.accent2.withAlpha(0.3f));
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, zwui::Palette::dim);

    allKnobs = { &cutoff, &res, &envAmt, &keyTrack, &drive, &spaceSize, &spaceDecay, &spaceDark, &duck, &duckRel, &glide, &drift, &volume };
    for (int o = 0; o < 2; ++o) for (auto* k : { &osc[o].position, &osc[o].level, &osc[o].octave, &osc[o].semis, &osc[o].fine, &osc[o].unison, &osc[o].spread, &osc[o].width }) allKnobs.push_back(k);
    for (int i = 0; i < 4; ++i) { allKnobs.push_back(&ampK[i]); allKnobs.push_back(&modK[i]); }
    for (int i = 0; i < 8; ++i) allKnobs.push_back(&macro[i]);
    installLocks();

    addAndMakeVisible(helpButton);
    helpButton.getProperties().set("chip", true); helpButton.getProperties().set("book", true);
    helpButton.onClick = [this] { manual.show(); };
    addChildComponent(manual);
    manual.set("NEBULA", "Planetary wavetable synth.", NebulaManual::sections());
    zwui::applyTooltips(NebulaManual::sections(), allKnobs);

    setResizable(true, true);
    setResizeLimits(832, 784, 1664, 1568);
    getConstrainer()->setFixedAspectRatio(double(kW) / double(kH));
    setSize(int(kW), int(kH));
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
}

NebulaAudioProcessorEditor::~NebulaAudioProcessorEditor() { setLookAndFeel(nullptr); }

void NebulaAudioProcessorEditor::installLocks()
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

void NebulaAudioProcessorEditor::syncModRings()
{
    const auto& targets = nebulaModTargets();
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

void NebulaAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit(0.0f, 0.1f, float((now - lastTick) * 0.001));
    lastTick = now;
    stage.tick(processor.motion, dt);
    lnf.pulse = processor.motion.pulse;
    presetBar.sync(); modPanel.tick(); syncModRings();
    planet.tick(dt);
    for (int o = 0; o < 2; ++o) { const float a = osc[o].on.getToggleState() ? 1.0f : 0.3f; for (auto* k : { &osc[o].position, &osc[o].level, &osc[o].octave, &osc[o].semis, &osc[o].fine, &osc[o].unison, &osc[o].spread, &osc[o].width }) k->slider.setAlpha(a); osc[o].table.setAlpha(a); }
    const bool sc = processor.sidechainPresent.load();
    duck.slider.setAlpha(sc ? 1.0f : 0.4f); duckRel.slider.setAlpha(sc ? 1.0f : 0.4f);
    repaint();
}

void NebulaAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getWidth() / kW;
    stage.render(g, getLocalBounds(), lnf.accent, lnf.accent2, processor.motion, 777);
    zwui::drawHeader(g, r, s, "NEBULA", "PLANETARY WAVETABLE SYNTH", lnf.accent, lnf.accentHi, lnf.accent2);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 88 * s, 250 * s, 336 * s), "OSC A", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(768 * s, 88 * s, 250 * s, 336 * s), "OSC B", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 436 * s, 500 * s, 176 * s), "CARVE  +  FILTER", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(532 * s, 436 * s, 486 * s, 176 * s), "ENVELOPES  +  SPACE", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 624 * s, 996 * s, 170 * s), "MOTION", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 806 * s, 996 * s, 96 * s), "MACROS", lnf.accent, lnf.accent2, s);
    g.setFont(juce::Font(juce::FontOptions(8.5f * s, juce::Font::bold))); g.setColour(lnf.accentHi);
    g.drawText("CARVE: click the halo to place a band (up to 8) - drag = harmonic / gain, Shift = width, double-click = remove", juce::Rectangle<float>(40 * s, 452 * s, 470 * s, 12 * s), juce::Justification::centredLeft);
    g.drawText("AMP", juce::Rectangle<float>(548 * s, 452 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
    g.drawText("MOD", juce::Rectangle<float>(780 * s, 452 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
    g.drawText("SPACE", juce::Rectangle<float>(548 * s, 538 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
    g.drawText(juce::String("DUCK (SC)") + (processor.sidechainPresent.load() ? "" : "  -  no input"), juce::Rectangle<float>(724 * s, 538 * s, 120 * s, 12 * s), juce::Justification::centredLeft);
    g.drawText("GLUE", juce::Rectangle<float>(842 * s, 538 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawVerticalLine(int(772 * s), 452 * s, 528 * s); g.drawVerticalLine(int(716 * s), 540 * s, 604 * s); g.drawVerticalLine(int(834 * s), 540 * s, 604 * s);
    zwui::drawOrbit(g, { 520 * s, 42 * s }, 24 * s, processor.motion, lnf.accent, lnf.accent2);
}

void NebulaAudioProcessorEditor::resized()
{
    const float s = float(getWidth()) / kW;
    auto S = [s](float v) { return juce::roundToInt(v * s); };
    presetBar.setBounds(0, 0, getWidth(), S(60));
    presetBar.layoutScaled(s, 584);
    helpButton.setBounds(S(936), S(61), S(82), S(18));
    manual.setBounds(getLocalBounds());
    powerButton.setBounds(S(984), S(20), S(34), S(34));

    planet.setBounds(S(276), S(80), S(488), S(348));
    for (int o = 0; o < 2; ++o)
    {
        auto& u = osc[o]; const int x0 = S(o == 0 ? 22 : 768);
        u.table.setBounds(x0 + S(14), S(108), S(150), S(22));
        u.on.setBounds(x0 + S(212), S(107), S(24), S(24));
        u.position.layout(x0 + S(12), S(154), S(112), S(120), S(14));
        u.octave.layout(x0 + S(132), S(150), S(50), S(50), S(10));
        u.semis.layout (x0 + S(188), S(150), S(50), S(50), S(10));
        u.fine.layout  (x0 + S(132), S(216), S(50), S(50), S(10));
        u.level.layout (x0 + S(188), S(216), S(50), S(50), S(10));
        u.unison.layout(x0 + S(12),  S(306), S(72), S(96), S(12));
        u.spread.layout(x0 + S(90),  S(306), S(72), S(96), S(12));
        u.width.layout (x0 + S(168), S(306), S(72), S(96), S(12));
    }
    carveButton.setBounds(S(40), S(470), S(70), S(24));
    cutoff.layout(S(130), S(486), S(104), S(112), S(14));
    res.layout(S(242), S(500), S(64), S(90), S(12)); envAmt.layout(S(310), S(500), S(64), S(90), S(12));
    keyTrack.layout(S(378), S(500), S(64), S(90), S(12)); drive.layout(S(446), S(500), S(64), S(90), S(12));
    filterModeBox.setBounds(S(40), S(504), S(80), S(22));
    for (int i = 0; i < 4; ++i) { ampK[i].layout(S(548 + i * 54), S(474), S(50), S(50), S(10)); modK[i].layout(S(780 + i * 54), S(474), S(50), S(50), S(10)); }
    zwui::Knob* row2[8] = { &spaceSize, &spaceDecay, &spaceDark, &duck, &duckRel, &glide, &drift, &volume };
    const int row2x[8] = { 548, 602, 656, 724, 778, 842, 896, 950 };
    for (int i = 0; i < 8; ++i) row2[i]->layout(S(row2x[i]), S(558), S(50), S(44), S(10));
    modPanel.setBounds(S(22), S(624), S(996), S(170));
    modPanel.layoutScaled(996.0f * s / 836.0f);
    for (int i = 0; i < 8; ++i) macro[i].layout(S(118 + i * 110), S(828), S(96), S(68), S(12));
    keyboard.setBounds(S(22), S(910), S(996), S(60));
}
