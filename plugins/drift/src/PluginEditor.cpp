#include "PluginEditor.h"

// ---------------------------------------------------------------------------
// Look and feel
// ---------------------------------------------------------------------------
ZenWaveLookAndFeel::ZenWaveLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::Label::textColourId, text);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::ComboBox::outlineColourId, line);
    setColour(juce::ComboBox::arrowColourId, violetHi);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, violet);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::ToggleButton::textColourId, dim);
    setColour(juce::ToggleButton::tickColourId, violetHi);
}

void ZenWaveLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(6.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcW = juce::jmax(2.5f, radius * 0.11f);

    // Track
    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius - arcW, radius - arcW, 0.0f, startAngle, endAngle, true);
    g.setColour(line);
    g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc (bipolar sliders fill from centre)
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    juce::Path value;
    value.addCentredArc(centre.x, centre.y, radius - arcW, radius - arcW, 0.0f, juce::jmin(from, angle), juce::jmax(from, angle), true);
    g.setColour(violet);
    g.strokePath(value, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Body
    const float bodyR = radius - arcW * 2.6f;
    g.setGradientFill(juce::ColourGradient(panel.brighter(0.25f), centre.x, centre.y - bodyR, panel.darker(0.4f), centre.x, centre.y + bodyR, false));
    g.fillEllipse(centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour(line.brighter(0.2f));
    g.drawEllipse(centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Pointer
    juce::Path p;
    p.addRoundedRectangle(-arcW * 0.5f, -bodyR + 4.0f, arcW, bodyR * 0.45f, arcW * 0.5f);
    g.setColour(violetHi);
    g.fillPath(p, juce::AffineTransform::rotation(angle).translated(centre));
}

void ZenWaveLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hover, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = b.getToggleState();
    g.setColour(on ? violet.withAlpha(0.25f) : panel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(on ? violetHi : (hover ? dim : line));
    g.drawRoundedRectangle(r, 6.0f, 1.0f);
    g.setColour(on ? juce::Colours::white : dim);
    g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    g.drawText(b.getButtonText(), r, juce::Justification::centred);
}

void ZenWaveLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0, 0, float(w), float(h)).reduced(0.5f);
    g.setColour(panel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(box.hasKeyboardFocus(true) ? violetHi : line);
    g.drawRoundedRectangle(r, 6.0f, 1.0f);
    juce::Path arrow;
    const float ax = float(w) - 14.0f, ay = float(h) * 0.5f;
    arrow.addTriangle(ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour(violetHi);
    g.fillPath(arrow);
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------
DriftAudioProcessorEditor::DriftAudioProcessorEditor(DriftAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);

    makeKnob(time,     ParamIDs::time,       "TIME",     true);
    makeKnob(feedback, ParamIDs::feedback,   "FEEDBACK", true);
    makeKnob(wobble,   ParamIDs::wowFlutter, "WOBBLE",   true);
    makeKnob(age,      ParamIDs::age,        "AGE",      true);
    makeKnob(space,    ParamIDs::space,      "SPACE",    true);
    makeKnob(mix,      ParamIDs::mix,        "MIX",      true);

    makeKnob(size,   ParamIDs::size,   "SIZE",   false);
    makeKnob(decay,  ParamIDs::decay,  "DECAY",  false);
    makeKnob(width,  ParamIDs::width,  "WIDTH",  false);
    makeKnob(spread, ParamIDs::spread, "SPREAD", false);
    makeKnob(output, ParamIDs::output, "OUTPUT", false);

    for (auto* b : { &syncButton, &pingPongButton, &freezeButton }) addAndMakeVisible(b);
    syncAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::sync, syncButton);
    pingAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::pingPong, pingPongButton);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, ParamIDs::freeze, freezeButton);

    addAndMakeVisible(divisionBox);
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processor.apvts.getParameter(ParamIDs::division)))
        divisionBox.addItemList(choice->choices, 1);
    divisionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, ParamIDs::division, divisionBox);

    addAndMakeVisible(presetBox);
    refreshPresetBox();
    presetBox.onChange = [this] {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) processor.setCurrentProgram(idx);
    };

    // Starfield
    for (int i = 0; i < 140; ++i) stars.push_back({ rng.nextFloat(), rng.nextFloat() });

    setResizable(true, true);
    setResizeLimits(640, 400, 1280, 800);
    getConstrainer()->setFixedAspectRatio(1.6);
    setSize(800, 500);
    startTimerHz(10);
}

DriftAudioProcessorEditor::~DriftAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void DriftAudioProcessorEditor::makeKnob(Knob& k, const char* paramId, const juce::String& title, bool big)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, big ? 72 : 60, 16);
    k.slider.setPopupDisplayEnabled(false, false, this);
    addAndMakeVisible(k.slider);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, paramId, k.slider);

    k.label.setText(title, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(juce::Font(juce::FontOptions(big ? 12.0f : 10.5f, juce::Font::bold)));
    k.label.setColour(juce::Label::textColourId, big ? ZenWaveLookAndFeel::text : ZenWaveLookAndFeel::dim);
    addAndMakeVisible(k.label);
}

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
    if (processor.getCurrentProgram() != lastPreset)
    {
        lastPreset = processor.getCurrentProgram();
        presetBox.setSelectedId(lastPreset + 1, juce::dontSendNotification);
    }
    divisionBox.setEnabled(syncButton.getToggleState());
    time.slider.setAlpha(syncButton.getToggleState() ? 0.35f : 1.0f);
}

void DriftAudioProcessorEditor::drawStarfield(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    for (size_t i = 0; i < stars.size(); ++i)
    {
        const float sz = 0.6f + float(i % 5) * 0.35f;
        const float a  = 0.15f + float(i % 7) * 0.08f;
        g.setColour(juce::Colours::white.withAlpha(a));
        g.fillEllipse(stars[i].x * b.getWidth(), stars[i].y * b.getHeight(), sz, sz);
    }
    // Violet nebula glow, bottom-right
    g.setGradientFill(juce::ColourGradient(ZenWaveLookAndFeel::violet.withAlpha(0.18f), b.getRight(), b.getBottom(),
                                           juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY() * 0.6f, true));
    g.fillRect(b);
}

void DriftAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(ZenWaveLookAndFeel::bg);
    drawStarfield(g);

    auto r = getLocalBounds().toFloat();
    const float s = r.getWidth() / 800.0f;

    // Header
    g.setColour(ZenWaveLookAndFeel::text);
    g.setFont(juce::Font(juce::FontOptions(26.0f * s, juce::Font::bold)));
    g.drawText("DRIFT", juce::Rectangle<float>(22 * s, 14 * s, 200 * s, 32 * s), juce::Justification::centredLeft);
    g.setColour(ZenWaveLookAndFeel::dim);
    g.setFont(juce::Font(juce::FontOptions(11.0f * s)));
    g.drawText("LO-FI SPACE ECHO   ·   ZENWAVE LABS", juce::Rectangle<float>(22 * s, 44 * s, 400 * s, 16 * s), juce::Justification::centredLeft);

    // Panel separators
    g.setColour(ZenWaveLookAndFeel::line);
    g.drawLine(20 * s, 72 * s, r.getWidth() - 20 * s, 72 * s, 1.0f);
    g.drawLine(20 * s, 332 * s, r.getWidth() - 20 * s, 332 * s, 1.0f);

    g.setColour(ZenWaveLookAndFeel::dim);
    g.setFont(juce::Font(juce::FontOptions(10.0f * s, juce::Font::bold)));
    g.drawText("DEEP CONTROL", juce::Rectangle<float>(22 * s, 338 * s, 200 * s, 14 * s), juce::Justification::centredLeft);
}

void DriftAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();
    const float s = r.getWidth() / 800.0f;
    auto S = [s](int v) { return juce::roundToInt(v * s); };

    presetBox.setBounds(S(470), S(22), S(250), S(28));
    freezeButton.setBounds(S(728), S(22), S(52), S(28));

    // Macro row: six big knobs
    const int bigW = S(120), bigH = S(150), bigY = S(100);
    Knob* bigs[] = { &time, &feedback, &wobble, &age, &space, &mix };
    int x = S(22);
    for (auto* k : bigs)
    {
        k->label.setBounds(x, bigY - S(18), bigW, S(16));
        k->slider.setBounds(x, bigY, bigW, bigH);
        x += S(126);
    }

    // Sync + division sit under TIME
    syncButton.setBounds(S(30), S(262), S(50), S(24));
    divisionBox.setBounds(S(84), S(262), S(60), S(24));
    pingPongButton.setBounds(S(152), S(262), S(90), S(24));

    // Deep row: five small knobs
    const int smW = S(96), smH = S(110), smY = S(368);
    Knob* smalls[] = { &size, &decay, &width, &spread, &output };
    x = S(22);
    for (auto* k : smalls)
    {
        k->label.setBounds(x, smY - S(14), smW, S(14));
        k->slider.setBounds(x, smY, smW, smH);
        x += S(104);
    }
}
