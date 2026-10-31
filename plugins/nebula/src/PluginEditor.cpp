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
    // ---- FX page ----
    for (auto* t : { &voiceTab, &fxTab }) { addAndMakeVisible(t); t->getProperties().set("chip", true); }
    voiceTab.onClick = [this] { setPage(0); }; fxTab.onClick = [this] { setPage(1); };
    voiceTab.setTooltip("Voice page: carve, filter, envelopes, space"); fxTab.setTooltip("FX page: EQ and the internal chain (Saturate -> Glue -> Echo), before Space");
    for (int i = 0; i < 4; ++i) { addAndMakeVisible(eqSel[i]); eqSel[i].setButtonText(juce::String(i + 1)); eqSel[i].onClick = [this, i] { attachEq(i); }; eqSel[i].setTooltip("Select EQ node " + juce::String(i + 1)); }
    addAndMakeVisible(eqOnButton); eqOnButton.getProperties().set("round", true); eqOnButton.setTooltip("Selected node on/off");
    addAndMakeVisible(eqTypeBox); eqTypeBox.addItemList({ "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut" }, 1); eqTypeBox.setTooltip("Node type");
    eqFreq.init(*this, s, "eq1freq", "FREQ", false, false); eqGain.init(*this, s, "eq1gain", "GAIN", false, false); eqQ.init(*this, s, "eq1q", "Q", false, false);
    attachEq(0);
    for (auto* b : { &satButton, &glueButton, &echoButton, &echoSyncButton, &echoPPButton }) addAndMakeVisible(b);
    satAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::satOn, satButton);
    glueAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::glueOn, glueButton);
    echoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::echoOn, echoButton);
    echoSyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::echoSync, echoSyncButton);
    echoPPAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::echoPP, echoPPButton);
    satButton.setTooltip("Saturate: Core-style tape stage on the whole patch"); glueButton.setTooltip("Glue: a single-band Pulse compressor - upward and downward, adaptive. Pads stay even, plucks get dense.");
    echoButton.setTooltip("Echo: synced stereo delay with a filtered, saturating feedback loop"); echoSyncButton.setTooltip("Echo time follows the host tempo"); echoPPButton.setTooltip("Ping-pong: repeats alternate left / right");
    addAndMakeVisible(echoDivBox); echoDivBox.addItemList(zwui::divisionNames(), 1); echoDivBox.setTooltip("Echo division when SYNC is on");
    echoDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, ParamIDs::echoDiv, echoDivBox);
    satDrive.init(*this, s, ParamIDs::satDrive, "DRIVE", false, false); satMix.init(*this, s, ParamIDs::satMix, "MIX", false, false);
    glueDepth.init(*this, s, ParamIDs::glueDepth, "DEPTH", false, false); glueTime.init(*this, s, ParamIDs::glueTime, "TIME", false, false);
    echoTime.init(*this, s, ParamIDs::echoTime, "TIME", false, false); echoFb.init(*this, s, ParamIDs::echoFb, "FDBK", false, false);
    echoMix.init(*this, s, ParamIDs::echoMix, "MIX", false, false); echoTone.init(*this, s, ParamIDs::echoTone, "TONE", false, false);

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

    allKnobs = { &cutoff, &res, &envAmt, &keyTrack, &drive, &spaceSize, &spaceDecay, &spaceDark, &duck, &duckRel, &glide, &drift, &volume,
                 &satDrive, &satMix, &glueDepth, &glueTime, &echoTime, &echoFb, &echoMix, &echoTone, &eqFreq, &eqGain, &eqQ };
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

    setPage(0);
    setResizable(true, true);
    setResizeLimits(832, 784, 1664, 1568);
    getConstrainer()->setFixedAspectRatio(double(kW) / double(kH));
    setSize(int(kW), int(kH));
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(30);
}

NebulaAudioProcessorEditor::~NebulaAudioProcessorEditor() { setLookAndFeel(nullptr); }

void NebulaAudioProcessorEditor::setPage(int p)
{
    page = p;
    const bool voice = p == 0;
    for (auto* c : std::initializer_list<juce::Component*>{ &carveButton, &cutoff.slider, &cutoff.label, &res.slider, &res.label, &envAmt.slider, &envAmt.label, &keyTrack.slider, &keyTrack.label, &drive.slider, &drive.label, &filterModeBox,
                                                            &spaceSize.slider, &spaceSize.label, &spaceDecay.slider, &spaceDecay.label, &spaceDark.slider, &spaceDark.label, &duck.slider, &duck.label, &duckRel.slider, &duckRel.label,
                                                            &glide.slider, &glide.label, &drift.slider, &drift.label, &volume.slider, &volume.label })
        c->setVisible(voice);
    for (int i = 0; i < 4; ++i) { ampK[i].slider.setVisible(voice); ampK[i].label.setVisible(voice); modK[i].slider.setVisible(voice); modK[i].label.setVisible(voice); eqSel[i].setVisible(! voice); }
    for (auto* c : std::initializer_list<juce::Component*>{ &eqOnButton, &eqTypeBox, &eqFreq.slider, &eqFreq.label, &eqGain.slider, &eqGain.label, &eqQ.slider, &eqQ.label,
                                                            &satButton, &glueButton, &echoButton, &echoSyncButton, &echoPPButton, &echoDivBox,
                                                            &satDrive.slider, &satDrive.label, &satMix.slider, &satMix.label, &glueDepth.slider, &glueDepth.label, &glueTime.slider, &glueTime.label,
                                                            &echoTime.slider, &echoTime.label, &echoFb.slider, &echoFb.label, &echoMix.slider, &echoMix.label, &echoTone.slider, &echoTone.label })
        c->setVisible(! voice);
    voiceTab.setToggleState(voice, juce::dontSendNotification); fxTab.setToggleState(! voice, juce::dontSendNotification);
    repaint();
}

void NebulaAudioProcessorEditor::attachEq(int i)
{
    selectedEq = i;
    auto& s = processor.apvts;
    eqFreq.attach.reset(); eqGain.attach.reset(); eqQ.attach.reset(); eqOnAttach.reset(); eqTypeAttach.reset();
    eqFreq.paramId = ParamIDs::eqFreq(i); eqGain.paramId = ParamIDs::eqGain(i); eqQ.paramId = ParamIDs::eqQ(i);
    eqFreq.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, ParamIDs::eqFreq(i), eqFreq.slider);
    eqGain.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, ParamIDs::eqGain(i), eqGain.slider);
    eqQ.attach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, ParamIDs::eqQ(i), eqQ.slider);
    eqOnAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, ParamIDs::eqOn(i), eqOnButton);
    eqTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, ParamIDs::eqType(i), eqTypeBox);
    for (int k = 0; k < 4; ++k) eqSel[k].setToggleState(k == i, juce::dontSendNotification);
}

static float eqX(float hz, juce::Rectangle<float> r) { return r.getX() + r.getWidth() * std::log(hz / 20.0f) / std::log(1000.0f); }
static float eqHz(float x, juce::Rectangle<float> r) { return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
static float eqY(float db, juce::Rectangle<float> r) { return zwui::eqFieldYForDb(r, db); }

void NebulaAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (page != 1 || ! eqDisplay.contains(e.getPosition())) return;
    const auto r = eqDisplay.toFloat(); eqDrag = -1;
    for (int i = 0; i < 4; ++i)
    {
        auto* f = processor.apvts.getParameter(ParamIDs::eqFreq(i)); auto* g = processor.apvts.getParameter(ParamIDs::eqGain(i));
        const juce::Point<float> c(eqX(f->convertFrom0to1(f->getValue()), r), eqY(g->convertFrom0to1(g->getValue()), r));
        if (c.getDistanceFrom(e.position) < 10.0f) { eqDrag = i; break; }
    }
    if (eqDrag < 0)   // click on empty display: place the first off node here, or move the selected one
    {
        for (int i = 0; i < 4; ++i) if (processor.apvts.getParameter(ParamIDs::eqOn(i))->getValue() < 0.5f) { eqDrag = i; auto* on = processor.apvts.getParameter(ParamIDs::eqOn(i)); on->beginChangeGesture(); on->setValueNotifyingHost(1.0f); on->endChangeGesture(); break; }
        if (eqDrag < 0) eqDrag = selectedEq;
    }
    attachEq(eqDrag);
    for (auto id : { ParamIDs::eqFreq(eqDrag), ParamIDs::eqGain(eqDrag) }) processor.apvts.getParameter(id)->beginChangeGesture();
    mouseDrag(e);
}
void NebulaAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (eqDrag < 0) return;
    const auto r = eqDisplay.toFloat();
    auto* f = processor.apvts.getParameter(ParamIDs::eqFreq(eqDrag)); auto* g = processor.apvts.getParameter(ParamIDs::eqGain(eqDrag));
    f->setValueNotifyingHost(f->convertTo0to1(eqHz(e.position.x, r)));
    g->setValueNotifyingHost(g->convertTo0to1(juce::jlimit(-18.0f, 18.0f, zwui::eqFieldDbForY(r, e.position.y))));
}
void NebulaAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    if (eqDrag < 0) return;
    for (auto id : { ParamIDs::eqFreq(eqDrag), ParamIDs::eqGain(eqDrag) }) processor.apvts.getParameter(id)->endChangeGesture();
    eqDrag = -1;
}

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
    if (page == 1) fxAnalyser.update(processor.spectrum, processor.currentSampleRate);
    { const bool sync = echoSyncButton.getToggleState(); echoDivBox.setEnabled(sync); echoDivBox.setAlpha(sync ? 1.0f : 0.4f); echoTime.slider.setAlpha(sync ? 0.35f : 1.0f); }
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
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 436 * s, 500 * s, 176 * s), page == 0 ? "CARVE  +  FILTER" : "EQ", lnf.accent, lnf.accent2, s);
    zwui::drawCard(g, juce::Rectangle<float>(532 * s, 436 * s, 486 * s, 176 * s), page == 0 ? "ENVELOPES  +  SPACE" : "CHAIN   SATURATE  ->  GLUE  ->  ECHO  ->  SPACE", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 624 * s, 996 * s, 170 * s), "MOTION", lnf.accent2, lnf.accent, s);
    zwui::drawCard(g, juce::Rectangle<float>(22 * s, 806 * s, 996 * s, 96 * s), "MACROS", lnf.accent, lnf.accent2, s);
    g.setFont(juce::Font(juce::FontOptions(8.5f * s, juce::Font::bold))); g.setColour(lnf.accentHi);
    if (page == 0)
    {
        g.drawText("CARVE: click the halo to place a band (up to 8) - drag = harmonic / gain, Shift = width, double-click = remove", juce::Rectangle<float>(40 * s, 452 * s, 470 * s, 12 * s), juce::Justification::centredLeft);
        g.drawText("AMP", juce::Rectangle<float>(548 * s, 452 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
        g.drawText("MOD", juce::Rectangle<float>(780 * s, 452 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
        g.drawText("SPACE", juce::Rectangle<float>(548 * s, 538 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
        g.drawText(juce::String("DUCK (SC)") + (processor.sidechainPresent.load() ? "" : "  -  no input"), juce::Rectangle<float>(724 * s, 538 * s, 120 * s, 12 * s), juce::Justification::centredLeft);
        g.drawText("GLUE", juce::Rectangle<float>(842 * s, 538 * s, 60 * s, 12 * s), juce::Justification::centredLeft);
        g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawVerticalLine(int(772 * s), 452 * s, 528 * s); g.drawVerticalLine(int(716 * s), 540 * s, 604 * s); g.drawVerticalLine(int(834 * s), 540 * s, 604 * s);
    }
    else
    {
        // EQ display: output spectrum + response curve + nodes
        const auto r = eqDisplay.toFloat();
        zwui::EqCurve flat { 20.0f, 20000.0f, 0, 0, 0.0f };
        zwui::drawEqField(g, r, fxAnalyser, nullptr, flat, lnf.accent, lnf.accent2, processor.motion.pulse, [this](float hz) { return processor.eqResponseDb(hz); }, "OUT");
        for (int i = 0; i < 4; ++i)
        {
            auto* f = processor.apvts.getParameter(ParamIDs::eqFreq(i)); auto* gp = processor.apvts.getParameter(ParamIDs::eqGain(i));
            const bool on = processor.apvts.getParameter(ParamIDs::eqOn(i))->getValue() > 0.5f;
            const juce::Point<float> c(eqX(f->convertFrom0to1(f->getValue()), r), eqY(gp->convertFrom0to1(gp->getValue()), r));
            if (! on) { g.setColour(zwui::Palette::dim.withAlpha(0.5f)); g.drawEllipse(c.x - 4, c.y - 4, 8, 8, 1.0f); continue; }
            g.setColour(i == selectedEq ? juce::Colours::white : lnf.accent2); g.fillEllipse(c.x - 6, c.y - 6, 12, 12);
            g.setColour(zwui::Palette::bg); g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold))); g.drawText(juce::String(i + 1), int(c.x) - 6, int(c.y) - 6, 12, 12, juce::Justification::centred);
        }
        g.setFont(juce::Font(juce::FontOptions(8.5f * s, juce::Font::bold))); g.setColour(lnf.accentHi);
        g.drawText("NODE " + juce::String(selectedEq + 1), juce::Rectangle<float>(352 * s, 452 * s, 100 * s, 12 * s), juce::Justification::centredLeft);
        g.setColour(zwui::Palette::dim); g.drawText("click the display to place / drag nodes", juce::Rectangle<float>(40 * s, 452 * s, 300 * s, 12 * s), juce::Justification::centredLeft);
        // chain readouts
        g.setColour(zwui::Palette::dim); g.setFont(juce::Font(juce::FontOptions(8.0f * s, juce::Font::bold)));
        const float gr = processor.glueGr();
        g.drawText("GR " + juce::String(gr > 0 ? "+" : "") + juce::String(gr, 1) + " dB", juce::Rectangle<float>(690 * s, 596 * s, 120 * s, 10 * s), juce::Justification::centredLeft);
        g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawVerticalLine(int(680 * s), 452 * s, 604 * s); g.drawVerticalLine(int(820 * s), 452 * s, 604 * s);
    }
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
    voiceTab.setBounds(S(846), S(428), S(70), S(18)); fxTab.setBounds(S(922), S(428), S(50), S(18));
    eqDisplay = juce::Rectangle<int>(S(40), S(468), S(300), S(128));
    for (int i = 0; i < 4; ++i) eqSel[i].setBounds(S(352 + i * 30), S(468), S(26), S(22));
    eqOnButton.setBounds(S(480), S(467), S(24), S(24));
    eqTypeBox.setBounds(S(352), S(496), S(152), S(22));
    eqFreq.layout(S(352), S(540), S(48), S(48), S(10)); eqGain.layout(S(404), S(540), S(48), S(48), S(10)); eqQ.layout(S(456), S(540), S(48), S(48), S(10));
    satButton.setBounds(S(548), S(468), S(90), S(22));
    satDrive.layout(S(548), S(522), S(50), S(50), S(10)); satMix.layout(S(604), S(522), S(50), S(50), S(10));
    glueButton.setBounds(S(690), S(468), S(70), S(22));
    glueDepth.layout(S(690), S(522), S(50), S(50), S(10)); glueTime.layout(S(746), S(522), S(50), S(50), S(10));
    echoButton.setBounds(S(830), S(468), S(60), S(22)); echoSyncButton.setBounds(S(894), S(468), S(50), S(22)); echoDivBox.setBounds(S(948), S(468), S(62), S(22));
    echoTime.layout(S(830), S(522), S(44), S(44), S(10)); echoFb.layout(S(878), S(522), S(44), S(44), S(10)); echoMix.layout(S(926), S(522), S(44), S(44), S(10)); echoTone.layout(S(974), S(522), S(40), S(44), S(10));
    echoPPButton.setBounds(S(830), S(580), S(90), S(18));
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
