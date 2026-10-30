#pragma once
// ZenWave UI — shared look-and-feel and widgets. Deep-space black, electric violet,
// instrument-panel knobs. Every plugin includes this so the family looks like a family.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zwui {

struct Palette
{
    static inline const juce::Colour bg       { 0xff0b0b10 };
    static inline const juce::Colour panel    { 0xff14141c };
    static inline const juce::Colour line     { 0xff2a2a38 };
    static inline const juce::Colour text     { 0xffe8e8f0 };
    static inline const juce::Colour dim      { 0xff8a8a9a };
    static inline const juce::Colour violet   { 0xff7c3aed };
    static inline const juce::Colour violetHi { 0xffa78bfa };
};

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit LookAndFeel(juce::Colour accent = Palette::violet, juce::Colour accentHi = Palette::violetHi)
        : accent(accent), accentHi(accentHi)
    {
        setColour(juce::ResizableWindow::backgroundColourId, Palette::bg);
        setColour(juce::Label::textColourId, Palette::text);
        setColour(juce::Slider::textBoxTextColourId, Palette::text);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, Palette::panel);
        setColour(juce::ComboBox::textColourId, Palette::text);
        setColour(juce::ComboBox::outlineColourId, Palette::line);
        setColour(juce::ComboBox::arrowColourId, accentHi);
        setColour(juce::PopupMenu::backgroundColourId, Palette::panel);
        setColour(juce::PopupMenu::textColourId, Palette::text);
        setColour(juce::PopupMenu::headerTextColourId, Palette::dim);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::ToggleButton::textColourId, Palette::dim);
        setColour(juce::ToggleButton::tickColourId, accentHi);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override
    {
        const auto bounds = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(6.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float arcW = juce::jmax(2.5f, radius * 0.11f);

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius - arcW, radius - arcW, 0.0f, startAngle, endAngle, true);
        g.setColour(Palette::line);
        g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        juce::Path value;
        value.addCentredArc(centre.x, centre.y, radius - arcW, radius - arcW, 0.0f, juce::jmin(from, angle), juce::jmax(from, angle), true);
        g.setColour(accent);
        g.strokePath(value, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float bodyR = radius - arcW * 2.6f;
        g.setGradientFill(juce::ColourGradient(Palette::panel.brighter(0.25f), centre.x, centre.y - bodyR,
                                               Palette::panel.darker(0.4f), centre.x, centre.y + bodyR, false));
        g.fillEllipse(centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour(Palette::line.brighter(0.2f));
        g.drawEllipse(centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        juce::Path p;
        p.addRoundedRectangle(-arcW * 0.5f, -bodyR + 4.0f, arcW, bodyR * 0.45f, arcW * 0.5f);
        g.setColour(accentHi);
        g.fillPath(p, juce::AffineTransform::rotation(angle).translated(centre));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.0f);
        const bool on = b.getToggleState();
        g.setColour(on ? accent.withAlpha(0.25f) : Palette::panel);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(on ? accentHi : (hover ? Palette::dim : Palette::line));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
        g.setColour(on ? juce::Colours::white : Palette::dim);
        g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
        g.drawText(b.getButtonText(), r, juce::Justification::centred);
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float>(0, 0, float(w), float(h)).reduced(0.5f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(box.hasKeyboardFocus(true) ? accentHi : Palette::line);
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
        juce::Path arrow;
        const float ax = float(w) - 14.0f, ay = float(h) * 0.5f;
        arrow.addTriangle(ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.setColour(accentHi);
        g.fillPath(arrow);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(juce::FontOptions(13.0f)); }
    juce::Font getLabelFont(juce::Label&) override { return juce::Font(juce::FontOptions(12.0f)); }

    juce::Colour accent, accentHi;
};

// A labelled rotary knob bound to an APVTS parameter.
struct Knob
{
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;

    void init(juce::Component& parent, juce::AudioProcessorValueTreeState& apvts, const char* paramId,
              const juce::String& title, bool big)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, big ? 72 : 60, 16);
        slider.setPopupDisplayEnabled(false, false, &parent);
        parent.addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
        label.setText(title, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(big ? 12.0f : 10.5f, juce::Font::bold)));
        label.setColour(juce::Label::textColourId, big ? Palette::text : Palette::dim);
        parent.addAndMakeVisible(label);
    }

    void layout(int x, int y, int w, int h, int labelH)
    {
        label.setBounds(x, y - labelH - 2, w, labelH);
        slider.setBounds(x, y, w, h);
    }
};

// Procedural starfield + nebula glow background, tinted by the plugin's accent.
inline void drawStarfield(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour accent, int seed = 42, int count = 140)
{
    juce::Random rng(seed);
    for (int i = 0; i < count; ++i)
    {
        const float sz = 0.6f + float(i % 5) * 0.35f;
        const float a  = 0.15f + float(i % 7) * 0.08f;
        g.setColour(juce::Colours::white.withAlpha(a));
        g.fillEllipse(rng.nextFloat() * b.getWidth(), rng.nextFloat() * b.getHeight(), sz, sz);
    }
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.18f), b.getRight(), b.getBottom(),
                                           juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY() * 0.6f, true));
    g.fillRect(b);
}

inline void drawHeader(juce::Graphics& g, juce::Rectangle<float> r, float s, const juce::String& title, const juce::String& subtitle)
{
    g.setColour(Palette::text);
    g.setFont(juce::Font(juce::FontOptions(26.0f * s, juce::Font::bold)));
    g.drawText(title, juce::Rectangle<float>(22 * s, 14 * s, 300 * s, 32 * s), juce::Justification::centredLeft);
    g.setColour(Palette::dim);
    g.setFont(juce::Font(juce::FontOptions(11.0f * s)));
    g.drawText(subtitle, juce::Rectangle<float>(22 * s, 44 * s, 420 * s, 16 * s), juce::Justification::centredLeft);
    g.setColour(Palette::line);
    g.drawLine(20 * s, 72 * s, r.getWidth() - 20 * s, 72 * s, 1.0f);
}

} // namespace zwui
