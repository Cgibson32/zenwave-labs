#pragma once
// ZenWave UI v4 — "deep space / new age / neon", now with depth and motion.
//  - Knobs: lit 3D bodies (specular, bevel, drop shadow), aurora value arcs, halos that
//    glow with value and breathe with the host tempo.
//  - Background: cached static layers (nebulae, planet horizon, geometry, grid) plus an
//    animated layer (twinkle, drifting nebula, beat pulse) drawn at ~30 fps.
//  - Orbit visualizer: particles orbiting a ring, driven by the plugin's output level.
//  - Power button: round, glows when the plugin is active.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace zwui {

struct Palette
{
    static inline const juce::Colour bg        { 0xff04040a };
    static inline const juce::Colour bg2       { 0xff0b0a18 };
    static inline const juce::Colour card      { 0xff0e0d1c };
    static inline const juce::Colour cardEdge  { 0xff26233f };
    static inline const juce::Colour knobDark  { 0xff07070f };
    static inline const juce::Colour knobLight { 0xff242246 };
    static inline const juce::Colour text      { 0xfff4f2ff };
    static inline const juce::Colour dim       { 0xff8784a6 };
    static inline const juce::Colour violet    { 0xff8b5cf6 };
    static inline const juce::Colour violetHi  { 0xffc4b5fd };
    static inline const juce::Colour cyan      { 0xff22d3ee };
    static inline const juce::Colour magenta   { 0xffff4fd8 };
};

// Shared animation state. The processor publishes levels and beat phase; the editor
// advances `time` on its timer and the LookAndFeel reads `pulse` for breathing halos.
struct Motion
{
    std::atomic<float> levelIn  { 0.0f };   // 0..1 dry level
    std::atomic<float> levelOut { 0.0f };   // 0..1 wet level
    std::atomic<float> beatPhase{ 0.0f };   // 0..1 within the current beat (host)
    std::atomic<float> bpm      { 120.0f };
    std::atomic<bool>  playing  { false };
    float time = 0.0f;                      // seconds, editor-side
    float pulse = 0.0f;                     // 0..1 breathing value, editor-side
};

inline juce::Colour aurora(float t, juce::Colour a, juce::Colour b)
{
    t = juce::jlimit(0.0f, 1.0f, t);
    if (t < 0.7f) return a.interpolatedWith(b, t / 0.7f);
    return b.interpolatedWith(juce::Colours::white, (t - 0.7f) / 0.3f * 0.45f);
}

inline void glowStroke(juce::Graphics& g, const juce::Path& p, juce::Colour c, float width, float glow)
{
    if (glow > 0.01f)
        for (int i = 4; i >= 1; --i)
        {
            g.setColour(c.withAlpha(0.09f * glow));
            g.strokePath(p, juce::PathStrokeType(width + i * 3.2f * glow, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit LookAndFeel(juce::Colour accent = Palette::violet, juce::Colour accentHi = Palette::violetHi, juce::Colour accent2 = Palette::cyan)
        : accent(accent), accentHi(accentHi), accent2(accent2)
    {
        setColour(juce::ResizableWindow::backgroundColourId, Palette::bg);
        setColour(juce::Label::textColourId, Palette::text);
        setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::outlineWhenEditingColourId, accent);
        setColour(juce::Label::backgroundWhenEditingColourId, Palette::bg2);
        setColour(juce::Label::textWhenEditingColourId, Palette::text);
        setColour(juce::Slider::textBoxTextColourId, accentHi);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxHighlightColourId, accent.withAlpha(0.4f));
        setColour(juce::ComboBox::backgroundColourId, Palette::card);
        setColour(juce::ComboBox::textColourId, Palette::text);
        setColour(juce::ComboBox::outlineColourId, Palette::cardEdge);
        setColour(juce::ComboBox::arrowColourId, accentHi);
        setColour(juce::PopupMenu::backgroundColourId, Palette::bg2);
        setColour(juce::PopupMenu::textColourId, Palette::text);
        setColour(juce::PopupMenu::headerTextColourId, accentHi);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha(0.75f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::ToggleButton::textColourId, Palette::dim);
        setColour(juce::ToggleButton::tickColourId, accentHi);
    }

    // Breathing amount (0..1) set by the editor each frame
    float pulse = 0.0f;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override
    {
        const auto bounds = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(9.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float arcW  = juce::jmax(3.0f, radius * 0.095f);
        const float arcR  = radius - arcW;
        const bool  hot   = s.isMouseOverOrDragging();
        const bool  bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        const float amount = bipolar ? std::abs(pos - 0.5f) * 2.0f : pos;
        const float breathe = 1.0f + pulse * 0.35f * amount;
        const float glow = (0.25f + amount * 1.3f + (hot ? 0.5f : 0.0f)) * breathe;
        const auto col = aurora(amount, accent, accent2);

        // Halo (value-scaled, breathing)
        if (amount > 0.02f)
        {
            const float hr = radius * (1.3f + 0.12f * pulse * amount);
            g.setGradientFill(juce::ColourGradient(col.withAlpha((0.20f * amount + (hot ? 0.1f : 0.0f)) * breathe), c.x, c.y,
                                                   juce::Colours::transparentBlack, c.x + hr, c.y, true));
            g.fillEllipse(c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);
        }

        // Drop shadow under the whole knob (depth)
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.55f), c.x, c.y + radius * 0.25f,
                                               juce::Colours::transparentBlack, c.x, c.y + radius * 1.25f, true));
        g.fillEllipse(c.x - radius * 1.1f, c.y - radius * 0.9f, radius * 2.2f, radius * 2.2f);

        // Bevelled outer ring (track groove)
        juce::Path track;
        track.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.strokePath(track, juce::PathStrokeType(arcW + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(Palette::cardEdge);
        g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(Palette::dim.withAlpha(0.35f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * i / 10.0f;
            const auto p1 = c.getPointOnCircumference(radius + 2.0f, a), p2 = c.getPointOnCircumference(radius + (i % 5 == 0 ? 6.0f : 4.0f), a);
            g.drawLine({ p1, p2 }, 1.0f);
        }

        // Value arc: aurora segments with glow
        if (std::abs(angle - from) > 0.002f)
        {
            const float a0 = juce::jmin(from, angle), a1 = juce::jmax(from, angle);
            const int segs = juce::jmax(6, int((a1 - a0) * 14.0f));
            juce::Path whole; whole.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
            for (int i = 4; i >= 1; --i)
            {
                g.setColour(col.withAlpha(0.08f * glow));
                g.strokePath(whole, juce::PathStrokeType(arcW + i * 3.0f * glow, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            for (int i = 0; i < segs; ++i)
            {
                const float t0 = float(i) / segs, t1 = float(i + 1) / segs;
                juce::Path seg; seg.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, a0 + (a1 - a0) * t0, a0 + (a1 - a0) * t1 + 0.01f, true);
                g.setColour(aurora(t1 * amount, accent, accent2));
                g.strokePath(seg, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
            }
            const auto tip = c.getPointOnCircumference(arcR, angle);
            g.setColour(juce::Colours::white.withAlpha(0.95f));
            g.fillEllipse(tip.x - arcW * 0.55f, tip.y - arcW * 0.55f, arcW * 1.1f, arcW * 1.1f);
        }

        // 3D body: dark base, lit top-left, specular highlight, bevel ring
        const float bodyR = arcR - arcW * 1.9f;
        g.setGradientFill(juce::ColourGradient(Palette::knobLight, c.x - bodyR * 0.5f, c.y - bodyR * 0.7f,
                                               Palette::knobDark, c.x + bodyR * 0.55f, c.y + bodyR * 0.9f, true));
        g.fillEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        // bevel: light rim top, dark rim bottom
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.22f), c.x, c.y - bodyR,
                                               juce::Colours::black.withAlpha(0.5f), c.x, c.y + bodyR, false));
        g.drawEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.6f);
        // specular
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.16f), c.x - bodyR * 0.35f, c.y - bodyR * 0.45f,
                                               juce::Colours::transparentBlack, c.x - bodyR * 0.35f + bodyR * 0.9f, c.y - bodyR * 0.45f + bodyR * 0.9f, true));
        g.fillEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        // inner accent ring + core light, value-driven
        g.setColour(col.withAlpha(0.15f + amount * 0.5f));
        g.drawEllipse(c.x - bodyR * 0.70f, c.y - bodyR * 0.70f, bodyR * 1.40f, bodyR * 1.40f, 1.2f);
        g.setGradientFill(juce::ColourGradient(col.withAlpha((0.10f + amount * 0.40f) * breathe), c.x, c.y,
                                               juce::Colours::transparentBlack, c.x + bodyR * 0.7f, c.y, true));
        g.fillEllipse(c.x - bodyR * 0.7f, c.y - bodyR * 0.7f, bodyR * 1.4f, bodyR * 1.4f);

        // Pointer with glow
        juce::Path ptr;
        ptr.addRoundedRectangle(-arcW * 0.42f, -bodyR + 5.0f, arcW * 0.84f, bodyR * 0.52f, arcW * 0.42f);
        ptr.applyTransform(juce::AffineTransform::rotation(angle).translated(c));
        for (int i = 3; i >= 1; --i)
        {
            g.setColour(col.withAlpha(0.10f + amount * 0.08f));
            g.strokePath(ptr, juce::PathStrokeType(i * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour(juce::Colours::white.interpolatedWith(accentHi, 0.3f));
        g.fillPath(ptr);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.0f);
        const bool on = b.getToggleState();
        const bool round = b.getProperties()["round"];
        const float rad = round ? r.getHeight() * 0.5f : 8.0f;
        if (round) r = r.withSizeKeepingCentre(r.getHeight(), r.getHeight());
        // depth
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(r.translated(0, 2.0f), rad);
        if (on)
        {
            for (int i = 4; i >= 1; --i) { auto e = r.expanded(i * 2.2f); g.setColour(accent2.withAlpha(0.07f * (1.0f + pulse * 0.5f))); g.fillRoundedRectangle(e, round ? e.getHeight() * 0.5f : rad + i * 2.0f); }
            g.setGradientFill(juce::ColourGradient(accent, r.getX(), r.getY(), accent2.darker(0.2f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle(r, rad);
            g.setColour(juce::Colours::white.withAlpha(0.7f)); g.drawRoundedRectangle(r, rad, 1.0f);
            g.setColour(juce::Colours::white);
        }
        else
        {
            g.setGradientFill(juce::ColourGradient(Palette::knobLight, r.getX(), r.getY(), Palette::card, r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle(r, rad);
            g.setColour(hover ? accent.withAlpha(0.8f) : Palette::cardEdge); g.drawRoundedRectangle(r, rad, 1.0f);
            g.setColour(hover ? Palette::text : Palette::dim);
        }
        if (round)
        {
            // power glyph
            juce::Path p; const auto c = r.getCentre(); const float pr = r.getHeight() * 0.26f;
            p.addCentredArc(c.x, c.y, pr, pr, 0.0f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
            g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawLine(c.x, c.y - pr - 2.0f, c.x, c.y - 1.0f, 2.0f);
        }
        else
        {
            g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
            g.drawText(b.getButtonText(), r, juce::Justification::centred);
        }
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float>(0, 0, float(w), float(h)).reduced(0.5f);
        g.setColour(juce::Colours::black.withAlpha(0.4f)); g.fillRoundedRectangle(r.translated(0, 1.5f), 8.0f);
        g.setGradientFill(juce::ColourGradient(Palette::knobLight.withAlpha(0.9f), r.getX(), r.getY(), Palette::card, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 8.0f);
        g.setGradientFill(juce::ColourGradient(accent.withAlpha(box.isPopupActive() ? 1.0f : 0.5f), r.getX(), r.getY(), accent2.withAlpha(0.5f), r.getRight(), r.getBottom(), false));
        g.drawRoundedRectangle(r, 8.0f, 1.0f);
        juce::Path arrow;
        const float ax = float(w) - 15.0f, ay = float(h) * 0.5f;
        arrow.addTriangle(ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.setColour(accentHi); g.fillPath(arrow);
    }

    void drawLabel(juce::Graphics& g, juce::Label& l) override
    {
        const bool isValueBox = dynamic_cast<juce::Slider*>(l.getParentComponent()) != nullptr;
        if (! isValueBox) { LookAndFeel_V4::drawLabel(g, l); return; }
        auto r = l.getLocalBounds().toFloat().reduced(2.0f, 1.0f);
        g.setColour(Palette::bg.withAlpha(0.6f));
        g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
        g.setColour(accent.withAlpha(0.35f));
        g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
        if (! l.isBeingEdited())
        {
            g.setColour(l.isEnabled() ? accentHi : Palette::dim);
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText(l.getText(), r, juce::Justification::centred, false);
        }
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(juce::FontOptions(13.0f, juce::Font::bold)); }
    juce::Font getLabelFont(juce::Label&) override { return juce::Font(juce::FontOptions(12.0f)); }

    juce::Colour accent, accentHi, accent2;
};

struct Knob
{
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;

    void init(juce::Component& parent, juce::AudioProcessorValueTreeState& apvts, const char* paramId,
              const juce::String& title, bool big)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, big ? 80 : 66, 17);
        slider.setPopupDisplayEnabled(false, false, &parent);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        parent.addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
        label.setText(title, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(big ? 11.5f : 10.0f, juce::Font::bold)));
        label.setColour(juce::Label::textColourId, big ? Palette::text : Palette::dim);
        parent.addAndMakeVisible(label);
    }

    void layout(int x, int y, int w, int h, int labelH)
    {
        label.setBounds(x, y - labelH - 2, w, labelH);
        slider.setBounds(x, y, w, h);
    }
};

// ---- Background: static (cached) + animated layers ------------------------------------

inline void drawBackgroundStatic(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour accent, juce::Colour accent2)
{
    g.setGradientFill(juce::ColourGradient(Palette::bg2, b.getCentreX(), b.getY(), Palette::bg, b.getCentreX(), b.getBottom(), false));
    g.fillRect(b);
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.26f), b.getWidth() * 0.82f, b.getHeight() * 0.9f, juce::Colours::transparentBlack, b.getWidth() * 0.35f, b.getHeight() * 0.3f, true));
    g.fillRect(b);
    g.setGradientFill(juce::ColourGradient(accent2.withAlpha(0.14f), b.getWidth() * 0.12f, b.getHeight() * 0.18f, juce::Colours::transparentBlack, b.getWidth() * 0.55f, b.getHeight() * 0.6f, true));
    g.fillRect(b);
    g.setGradientFill(juce::ColourGradient(Palette::magenta.withAlpha(0.07f), b.getWidth() * 0.5f, b.getHeight() * 1.05f, juce::Colours::transparentBlack, b.getWidth() * 0.5f, b.getHeight() * 0.55f, true));
    g.fillRect(b);

    // Geometry rings
    g.setColour(juce::Colours::white.withAlpha(0.035f));
    const juce::Point<float> gc(b.getWidth() * 0.78f, b.getHeight() * 0.42f);
    for (int i = 1; i <= 5; ++i) g.drawEllipse(gc.x - i * 48.0f, gc.y - i * 48.0f, i * 96.0f, i * 96.0f, 1.0f);
    for (int i = 0; i < 6; ++i)
    {
        const auto p = gc.getPointOnCircumference(96.0f, i * juce::MathConstants<float>::twoPi / 6.0f);
        g.drawEllipse(p.x - 96.0f, p.y - 96.0f, 192.0f, 192.0f, 0.8f);
    }
    // Grid
    g.setColour(juce::Colours::white.withAlpha(0.022f));
    for (float gx = 0; gx < b.getWidth(); gx += 40.0f) g.drawVerticalLine(int(gx), b.getY(), b.getBottom());
    for (float gy = 0; gy < b.getHeight(); gy += 40.0f) g.drawHorizontalLine(int(gy), b.getX(), b.getRight());
}

// Animated: drifting nebula veil, twinkling stars, beat-pulsing horizon, parallax
inline void drawBackgroundAnimated(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour accent, juce::Colour accent2,
                                   const Motion& m, int seed = 42)
{
    const float t = m.time;
    // Drifting veil
    const float vx = b.getWidth() * (0.5f + 0.25f * std::sin(t * 0.07f)), vy = b.getHeight() * (0.5f + 0.2f * std::cos(t * 0.05f));
    g.setGradientFill(juce::ColourGradient(accent2.withAlpha(0.07f), vx, vy, juce::Colours::transparentBlack, vx + b.getWidth() * 0.45f, vy, true));
    g.fillRect(b);

    // Planet horizon with beat pulse
    {
        const float pulse = m.pulse;
        const float R = b.getWidth() * 1.6f;
        const juce::Point<float> pc(b.getCentreX(), b.getBottom() + R - b.getHeight() * 0.10f);
        juce::Path rim; rim.addCentredArc(pc.x, pc.y, R, R, 0.0f, -1.25f, 1.25f, true);
        glowStroke(g, rim, accent2.withAlpha(0.45f + 0.4f * pulse), 1.5f, 1.8f + 2.5f * pulse);
        g.setGradientFill(juce::ColourGradient(accent2.withAlpha(0.08f + 0.10f * pulse), pc.x, pc.y - R, juce::Colours::transparentBlack, pc.x, pc.y - R + b.getHeight() * (0.16f + 0.08f * pulse), false));
        g.fillEllipse(pc.x - R, pc.y - R, R * 2, R * 2);
        // a few orbiting satellites along the rim
        for (int i = 0; i < 3; ++i)
        {
            const float a = -1.1f + std::fmod(t * (0.05f + 0.02f * i) + i * 0.8f, 2.2f);
            const auto p = pc.getPointOnCircumference(R, a + juce::MathConstants<float>::pi * 0.0f);
            // getPointOnCircumference measures from 12 o'clock; our arc used -1.25..1.25 around the top
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.fillEllipse(p.x - 1.5f, p.y - 1.5f, 3.0f, 3.0f);
            g.setColour(accent2.withAlpha(0.25f));
            g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        }
    }

    // Stars: twinkle + slow parallax drift
    juce::Random rng(seed);
    for (int i = 0; i < 190; ++i)
    {
        const float depth = 0.3f + float(i % 4) * 0.25f;                 // parallax layers
        const float sz = 0.7f + float(i % 5) * 0.45f;
        const float baseA = 0.12f + float(i % 7) * 0.10f;
        const float tw = 0.65f + 0.35f * std::sin(t * (0.8f + float(i % 9) * 0.37f) + i);
        float sx = rng.nextFloat() * b.getWidth(), sy = rng.nextFloat() * b.getHeight();
        sx = std::fmod(sx + t * 2.0f * depth, b.getWidth());
        const auto col = (i % 13 == 0) ? accent2 : (i % 17 == 0 ? accent : juce::Colours::white);
        if (i % 23 == 0) { g.setColour(col.withAlpha(0.18f * tw)); g.fillEllipse(sx - 3, sy - 3, sz + 6, sz + 6); }
        g.setColour(col.withAlpha(baseA * tw));
        g.fillEllipse(sx, sy, sz, sz);
    }
}

// Orbit visualizer: a ring of particles whose radius and brightness follow the output level.
inline void drawOrbit(juce::Graphics& g, juce::Point<float> c, float radius, const Motion& m, juce::Colour accent, juce::Colour accent2)
{
    const float lvl = juce::jlimit(0.0f, 1.0f, m.levelOut.load() * 1.6f);
    const float t = m.time;
    // core
    g.setGradientFill(juce::ColourGradient(aurora(lvl, accent, accent2).withAlpha(0.25f + 0.5f * lvl), c.x, c.y, juce::Colours::transparentBlack, c.x + radius * (0.6f + lvl * 0.6f), c.y, true));
    g.fillEllipse(c.x - radius * 1.3f, c.y - radius * 1.3f, radius * 2.6f, radius * 2.6f);
    // rings
    for (int r = 0; r < 3; ++r)
    {
        const float rr = radius * (0.45f + 0.28f * r) * (1.0f + lvl * 0.25f);
        juce::Path ring; ring.addEllipse(c.x - rr, c.y - rr, rr * 2, rr * 2);
        g.setColour(aurora(0.3f * r + lvl * 0.5f, accent, accent2).withAlpha(0.18f + 0.3f * lvl));
        g.strokePath(ring, juce::PathStrokeType(1.0f));
        // particles
        const int n = 6 + r * 4;
        for (int i = 0; i < n; ++i)
        {
            const float a = t * (0.6f - 0.15f * r) * (r % 2 ? -1.0f : 1.0f) + juce::MathConstants<float>::twoPi * i / n;
            const auto p = c.getPointOnCircumference(rr, a);
            const float ps = 1.2f + lvl * 2.5f + (i % 3 == 0 ? 0.8f : 0.0f);
            g.setColour(juce::Colours::white.withAlpha(0.35f + 0.6f * lvl));
            g.fillEllipse(p.x - ps * 0.5f, p.y - ps * 0.5f, ps, ps);
        }
    }
}

inline void drawCard(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour accent, juce::Colour accent2, float s)
{
    // drop shadow
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(r.translated(0, 4.0f * s), 16.0f * s);
    g.setColour(Palette::card.withAlpha(0.74f));
    g.fillRoundedRectangle(r, 16.0f * s);
    // top sheen
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.06f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + r.getHeight() * 0.35f, false));
    g.fillRoundedRectangle(r, 16.0f * s);
    // inner shadow at the bottom
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, r.getX(), r.getBottom() - r.getHeight() * 0.3f, juce::Colours::black.withAlpha(0.35f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 16.0f * s);
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.7f), r.getX(), r.getY(), accent2.withAlpha(0.35f), r.getRight(), r.getBottom(), false));
    g.drawRoundedRectangle(r, 16.0f * s, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour(accent2.brighter(0.2f).withAlpha(0.95f));
        g.setFont(juce::Font(juce::FontOptions(9.5f * s, juce::Font::bold)));
        g.drawText(title, r.reduced(16.0f * s, 9.0f * s), juce::Justification::topLeft);
    }
}

inline void drawHeader(juce::Graphics& g, juce::Rectangle<float> r, float s, const juce::String& title,
                       const juce::String& subtitle, juce::Colour accent, juce::Colour accentHi, juce::Colour accent2)
{
    const auto tr = juce::Rectangle<float>(26 * s, 12 * s, 360 * s, 40 * s);
    g.setFont(juce::Font(juce::FontOptions(32.0f * s, juce::Font::bold)));
    for (int i = 3; i >= 1; --i) { g.setColour(accent.withAlpha(0.10f)); g.drawText(title, tr.expanded(i * 1.0f), juce::Justification::centredLeft); }
    g.setGradientFill(juce::ColourGradient(Palette::text, tr.getX(), tr.getY(), accentHi, tr.getX() + 180 * s, tr.getBottom(), false));
    g.drawText(title, tr, juce::Justification::centredLeft);
    juce::Path ul; ul.startNewSubPath(26 * s, 56 * s); ul.lineTo(26 * s + 64 * s, 56 * s);
    glowStroke(g, ul, accent2, 2.0f * s, 1.2f);
    g.setColour(Palette::dim);
    g.setFont(juce::Font(juce::FontOptions(10.5f * s, juce::Font::bold)));
    g.drawText(subtitle, juce::Rectangle<float>(26 * s, 60 * s, 420 * s, 14 * s), juce::Justification::centredLeft);
    g.drawText("Z E N W A V E   L A B S", juce::Rectangle<float>(r.getWidth() - 330 * s, 60 * s, 304 * s, 14 * s), juce::Justification::centredRight);
}

// Editor helper: owns the cached static background and the animation clock.
class Stage
{
public:
    void render(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour accent, juce::Colour accent2, Motion& m, int seed)
    {
        if (cache.getWidth() != bounds.getWidth() || cache.getHeight() != bounds.getHeight())
        {
            cache = juce::Image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
            juce::Graphics cg(cache);
            drawBackgroundStatic(cg, bounds.toFloat(), accent, accent2);
        }
        g.drawImageAt(cache, 0, 0);
        drawBackgroundAnimated(g, bounds.toFloat(), accent, accent2, m, seed);
    }

    // Call from the editor timer: advances time and computes the breathing pulse.
    void tick(Motion& m, float dtSeconds)
    {
        m.time += dtSeconds;
        float target;
        if (m.playing.load())
        {
            const float ph = m.beatPhase.load();           // 0..1 per beat
            target = std::pow(1.0f - ph, 3.0f);            // sharp on the beat, decays
        }
        else
        {
            target = 0.5f + 0.5f * std::sin(m.time * 1.2f); // idle breathing (~0.2 Hz)
            target *= 0.5f;
        }
        m.pulse += (target - m.pulse) * (target > m.pulse ? 0.6f : 0.12f);
    }

private:
    juce::Image cache;
};

} // namespace zwui

#include <juce_dsp/juce_dsp.h>

namespace zwui {

// Editor-side analyser: FFT with smoothing, log-frequency bins ready to draw.
class SpectrumAnalyser
{
public:
    static constexpr int kOrder = 11, kSize = 1 << kOrder, kBins = 96;
    SpectrumAnalyser() : fft(kOrder), window(kSize, juce::dsp::WindowingFunction<float>::hann) { levels.fill(0.0f); }

    template <class Tap> void update(const Tap& tap, double sampleRate)
    {
        tap.snapshot(buf.data());
        window.multiplyWithWindowingTable(buf.data(), kSize);
        std::fill(fftData.begin(), fftData.end(), 0.0f);
        std::copy(buf.begin(), buf.end(), fftData.begin());
        fft.performFrequencyOnlyForwardTransform(fftData.data());
        for (int b = 0; b < kBins; ++b)
        {
            const float f0 = freqForBin(b), f1 = freqForBin(b + 1);
            int i0 = juce::jlimit(1, kSize / 2 - 1, int(f0 / sampleRate * kSize)), i1 = juce::jlimit(i0 + 1, kSize / 2, int(f1 / sampleRate * kSize) + 1);
            float pk = 0.0f; for (int i = i0; i < i1; ++i) pk = juce::jmax(pk, fftData[size_t(i)]);
            const float db = juce::Decibels::gainToDecibels(pk / (kSize * 0.25f) + 1e-9f);
            const float norm = juce::jlimit(0.0f, 1.0f, (db + 78.0f) / 78.0f);
            levels[size_t(b)] = norm > levels[size_t(b)] ? norm : levels[size_t(b)] * 0.86f + norm * 0.14f;
        }
    }
    static float freqForBin(int b) { return 20.0f * std::pow(1000.0f, float(b) / kBins); }   // 20 Hz .. 20 kHz
    std::array<float, kBins> levels;
private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, kSize> buf {};
    std::array<float, kSize * 2> fftData {};
};

struct EqCurve { float lowCutHz, highCutHz; int lowSlope, highSlope; float tilt; };

// The "field": log-frequency spectrum of the wet signal with the EQ response drawn over it.
inline void drawEqField(juce::Graphics& g, juce::Rectangle<float> r, const SpectrumAnalyser& an, const EqCurve& eq,
                        juce::Colour accent, juce::Colour accent2, float pulse)
{
    auto xFor = [&](float hz) { return r.getX() + r.getWidth() * std::log(hz / 20.0f) / std::log(1000.0f); };
    auto yForDb = [&](float db) { return r.getCentreY() - db / 24.0f * r.getHeight() * 0.5f; };  // ±24 dB range

    g.setColour(Palette::bg.withAlpha(0.55f));
    g.fillRoundedRectangle(r, 10.0f);
    // grid
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    for (float hz : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f }) g.drawVerticalLine(int(xFor(hz)), r.getY(), r.getBottom());
    for (float db : { -12.f, 0.f, 12.f }) g.drawHorizontalLine(int(yForDb(db)), r.getX(), r.getRight());
    g.setFont(juce::Font(juce::FontOptions(8.5f)));
    g.setColour(Palette::dim.withAlpha(0.8f));
    for (auto [hz, t] : std::initializer_list<std::pair<float, const char*>>{ { 100.f, "100" }, { 1000.f, "1k" }, { 10000.f, "10k" } })
        g.drawText(t, int(xFor(hz)) + 3, int(r.getBottom()) - 13, 30, 12, juce::Justification::left);

    // spectrum fill
    juce::Path spec;
    spec.startNewSubPath(r.getX(), r.getBottom());
    for (int b = 0; b < SpectrumAnalyser::kBins; ++b)
    {
        const float x = r.getX() + r.getWidth() * float(b) / (SpectrumAnalyser::kBins - 1);
        const float y = r.getBottom() - an.levels[size_t(b)] * r.getHeight() * 0.92f;
        if (b == 0) spec.lineTo(x, y); else spec.lineTo(x, y);
    }
    spec.lineTo(r.getRight(), r.getBottom()); spec.closeSubPath();
    g.setGradientFill(juce::ColourGradient(accent2.withAlpha(0.45f + 0.15f * pulse), r.getX(), r.getY(), accent.withAlpha(0.05f), r.getX(), r.getBottom(), false));
    g.fillPath(spec);
    g.setColour(accent2.withAlpha(0.8f));
    g.strokePath(spec, juce::PathStrokeType(1.2f));

    // EQ response
    juce::Path curve; bool first = true;
    for (int i = 0; i <= 160; ++i)
    {
        const float hz = 20.0f * std::pow(1000.0f, float(i) / 160.0f);
        const int nLow = 1 << eq.lowSlope, nHigh = 1 << eq.highSlope;                 // order: 1,2,4,8
        const float hpMag = 1.0f / std::sqrt(1.0f + std::pow(eq.lowCutHz / hz, 2.0f * nLow));
        const float lpMag = 1.0f / std::sqrt(1.0f + std::pow(hz / eq.highCutHz, 2.0f * nHigh));
        const float tiltDb = eq.tilt * 6.0f * std::tanh(std::log(hz / 1000.0f) * 0.9f);
        const float db = juce::Decibels::gainToDecibels(hpMag * lpMag + 1e-6f) + tiltDb;
        const float x = xFor(hz), y = juce::jlimit(r.getY(), r.getBottom(), yForDb(db));
        if (first) { curve.startNewSubPath(x, y); first = false; } else curve.lineTo(x, y);
    }
    glowStroke(g, curve, juce::Colours::white.interpolatedWith(accent, 0.25f), 2.0f, 1.0f + pulse * 0.5f);

    // cutoff markers
    for (float hz : { eq.lowCutHz, eq.highCutHz })
    {
        const float x = xFor(juce::jlimit(20.0f, 20000.0f, hz));
        g.setColour(accent.withAlpha(0.6f));
        g.drawVerticalLine(int(x), r.getY() + 4.0f, r.getBottom() - 4.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 3.0f, yForDb(-3.0f) - 3.0f, 6.0f, 6.0f);
    }
    g.setColour(accent.withAlpha(0.5f));
    g.drawRoundedRectangle(r, 10.0f, 1.0f);
}

} // namespace zwui
