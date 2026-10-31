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
    static inline const juce::Colour bg        { 0xff000000 };
    static inline const juce::Colour bg2       { 0xff050508 };
    static inline const juce::Colour card      { 0xff08080d };
    static inline const juce::Colour cardEdge  { 0xff1e1c2e };
    static inline const juce::Colour knobDark  { 0xff020204 };
    static inline const juce::Colour knobLight { 0xff1a1a24 };
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
    std::atomic<double> ppq     { 0.0 };    // host position in quarter notes
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
        const float arcW  = juce::jmax(3.0f, radius * 0.085f);
        const float arcR  = radius - arcW;
        const bool  hot   = s.isMouseOverOrDragging();
        const bool  bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        // Glow follows the modulated value when the knob is a live modulation target, so it pulses with the motion
        const float modPosEarly = float(s.getProperties().getWithDefault("modPos", -1.0f));
        const float glowPos = modPosEarly >= 0.0f ? modPosEarly : pos;
        const float amount = bipolar ? std::abs(glowPos - 0.5f) * 2.0f : glowPos;
        const float breathe = 1.0f + pulse * 0.35f * amount;
        const float glow = (0.25f + amount * 1.3f + (hot ? 0.5f : 0.0f)) * breathe;
        const auto col = aurora(amount, accent, accent2);

        // Halo
        if (amount > 0.02f)
        {
            const float hr = radius * (1.35f + 0.12f * pulse * amount);
            g.setGradientFill(juce::ColourGradient(col.withAlpha((0.22f * amount + (hot ? 0.1f : 0.0f)) * breathe), c.x, c.y,
                                                   juce::Colours::transparentBlack, c.x + hr, c.y, true));
            g.fillEllipse(c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);
        }
        // Drop shadow
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.6f), c.x, c.y + radius * 0.25f,
                                               juce::Colours::transparentBlack, c.x, c.y + radius * 1.25f, true));
        g.fillEllipse(c.x - radius * 1.1f, c.y - radius * 0.9f, radius * 2.2f, radius * 2.2f);

        // LED tick ring: 31 ticks, lit up to the value in the aurora gradient
        {
            const int ticks = 31;
            const float tickR0 = radius + 1.0f, tickR1 = radius + 5.0f;
            for (int i = 0; i < ticks; ++i)
            {
                const float t = float(i) / (ticks - 1);
                const float a = startAngle + (endAngle - startAngle) * t;
                const bool lit = bipolar ? (pos >= 0.5f ? (t >= 0.5f && t <= pos) : (t <= 0.5f && t >= pos)) : (t <= pos + 0.001f);
                const auto p1 = c.getPointOnCircumference(tickR0, a), p2 = c.getPointOnCircumference(tickR1 + (i % 5 == 0 ? 2.0f : 0.0f), a);
                if (lit) { g.setColour(aurora(t * amount, accent, accent2).withAlpha(0.35f)); g.drawLine({ p1, p2 }, 3.0f); }
                g.setColour(lit ? aurora(t, accent, accent2).brighter(0.2f) : Palette::dim.withAlpha(0.25f));
                g.drawLine({ p1, p2 }, lit ? 1.6f : 1.0f);
            }
        }

        // Track groove
        juce::Path track;
        track.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.65f));
        g.strokePath(track, juce::PathStrokeType(arcW + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(Palette::cardEdge);
        g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc
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

        // Cap: glossy obsidian with a violet rim light and a specular streak
        const float capR = arcR - arcW * 1.9f;
        // base: near-black radial with a lit upper-left
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c1b26), c.x - capR * 0.45f, c.y - capR * 0.6f,
                                               juce::Colour(0xff030306), c.x + capR * 0.4f, c.y + capR * 0.7f, true));
        g.fillEllipse(c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        // outer bevel: violet rim light at top, deep shadow at bottom
        g.setGradientFill(juce::ColourGradient(accentHi.withAlpha(0.55f), c.x, c.y - capR,
                                               juce::Colours::black.withAlpha(0.8f), c.x, c.y + capR, false));
        g.drawEllipse(c.x - capR + 0.75f, c.y - capR + 0.75f, capR * 2.0f - 1.5f, capR * 2.0f - 1.5f, 1.5f);
        // knurl band (fine, low contrast)
        for (int i = 0; i < 60; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * i / 60.0f;
            const auto p1 = c.getPointOnCircumference(capR * 0.84f, a), p2 = c.getPointOnCircumference(capR * 0.95f, a);
            g.setColour(juce::Colours::white.withAlpha(i % 2 ? 0.035f : 0.08f));
            g.drawLine({ p1, p2 }, 1.0f);
        }
        // dome: glossy black
        const float domeR = capR * 0.76f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff15141e), c.x - domeR * 0.3f, c.y - domeR * 0.5f,
                                               juce::Colour(0xff050508), c.x + domeR * 0.5f, c.y + domeR * 0.9f, true));
        g.fillEllipse(c.x - domeR, c.y - domeR, domeR * 2.0f, domeR * 2.0f);
        g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.45f), c.x, c.y - domeR, juce::Colours::transparentBlack, c.x, c.y + domeR * 0.2f, false));
        g.drawEllipse(c.x - domeR, c.y - domeR, domeR * 2.0f, domeR * 2.0f, 1.0f);
        // specular streak (curved highlight, upper-left)
        {
            juce::Path streak;
            streak.addCentredArc(c.x, c.y, domeR * 0.78f, domeR * 0.78f, 0.0f, -2.35f, -1.05f, true);
            g.setColour(juce::Colours::white.withAlpha(0.22f));
            g.strokePath(streak, juce::PathStrokeType(domeR * 0.10f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(juce::Colours::white.withAlpha(0.10f));
            g.strokePath(streak, juce::PathStrokeType(domeR * 0.22f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // violet reactor core, value-driven
        g.setGradientFill(juce::ColourGradient(col.withAlpha((0.12f + amount * 0.55f) * breathe), c.x, c.y + domeR * 0.15f,
                                               juce::Colours::transparentBlack, c.x + domeR * 0.72f, c.y, true));
        g.fillEllipse(c.x - domeR, c.y - domeR, domeR * 2.0f, domeR * 2.0f);
        g.setColour(col.withAlpha(0.25f + amount * 0.55f));
        g.drawEllipse(c.x - domeR * 0.5f, c.y - domeR * 0.5f, domeR, domeR, 1.0f);

        // Pointer
        juce::Path ptr;
        ptr.addRoundedRectangle(-arcW * 0.4f, -capR + 4.0f, arcW * 0.8f, capR * 0.55f, arcW * 0.4f);
        ptr.applyTransform(juce::AffineTransform::rotation(angle).translated(c));
        for (int i = 3; i >= 1; --i)
        {
            g.setColour(col.withAlpha(0.10f + amount * 0.08f));
            g.strokePath(ptr, juce::PathStrokeType(i * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour(juce::Colours::white.interpolatedWith(accentHi, 0.25f));
        g.fillPath(ptr);

        // Modulation: range arc outside the LED ring + a live cyan dot at the modulated position
        const float modPos = float(s.getProperties().getWithDefault("modPos", -1.0f));
        if (modPos >= 0.0f)
        {
            const float lo = float(s.getProperties().getWithDefault("modLo", 0.0f)), hi = float(s.getProperties().getWithDefault("modHi", 1.0f));
            const float mr = radius + 8.5f;
            juce::Path span; span.addCentredArc(c.x, c.y, mr, mr, 0.0f, startAngle + lo * (endAngle - startAngle), startAngle + hi * (endAngle - startAngle), true);
            g.setColour(accent2.withAlpha(0.22f)); g.strokePath(span, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(accent2.withAlpha(0.85f)); g.strokePath(span, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const auto dp = c.getPointOnCircumference(mr, startAngle + modPos * (endAngle - startAngle));
            g.setColour(accent2.withAlpha(0.35f)); g.fillEllipse(dp.x - 5.0f, dp.y - 5.0f, 10.0f, 10.0f);
            g.setColour(juce::Colours::white); g.fillEllipse(dp.x - 2.2f, dp.y - 2.2f, 4.4f, 4.4f);
        }
        // Lock glyph (Warp won't touch this knob)
        if (bool(s.getProperties().getWithDefault("locked", false)))
        {
            const auto lp = juce::Rectangle<float>(c.x + radius * 0.55f, c.y + radius * 0.62f, 9.0f, 7.0f);
            g.setColour(Palette::bg.withAlpha(0.8f)); g.fillRoundedRectangle(lp.expanded(3.0f), 4.0f);
            g.setColour(accentHi); g.fillRoundedRectangle(lp, 1.5f);
            juce::Path sh; sh.addCentredArc(lp.getCentreX(), lp.getY(), 3.0f, 3.5f, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
            g.strokePath(sh, juce::PathStrokeType(1.5f));
        }
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.0f);
        const bool on = b.getToggleState();
        const bool round = bool(b.getProperties()["round"]);
        if (round) r = r.withSizeKeepingCentre(juce::jmin(r.getWidth(), r.getHeight()), juce::jmin(r.getWidth(), r.getHeight()));
        auto shape = [&](juce::Rectangle<float> q) { juce::Path p; if (round) p.addEllipse(q); else p.addRoundedRectangle(q, 8.0f); return p; };

        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillPath(shape(r.translated(0, 2.0f)));
        if (on)
        {
            for (int i = 5; i >= 1; --i) { g.setColour(accent2.withAlpha(0.06f * (1.0f + pulse * 0.6f))); g.fillPath(shape(r.expanded(i * 2.4f))); }
            g.setGradientFill(juce::ColourGradient(accent, r.getX(), r.getY(), accent2.darker(0.2f), r.getRight(), r.getBottom(), false));
            g.fillPath(shape(r));
            g.setColour(juce::Colours::white.withAlpha(0.75f)); g.strokePath(shape(r), juce::PathStrokeType(1.0f));
            g.setColour(juce::Colours::white);
        }
        else
        {
            g.setGradientFill(juce::ColourGradient(Palette::knobLight, r.getX(), r.getY(), Palette::card, r.getX(), r.getBottom(), false));
            g.fillPath(shape(r));
            g.setColour(hover ? accent.withAlpha(0.8f) : Palette::cardEdge); g.strokePath(shape(r), juce::PathStrokeType(1.0f));
            g.setColour(hover ? Palette::text : Palette::dim);
        }
        if (round)
        {
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

    juce::Rectangle<int> getTooltipBounds(const juce::String& tip, juce::Point<int> pos, juce::Rectangle<int> parent) override
    {
        juce::AttributedString as; as.append(tip, juce::Font(juce::FontOptions(12.0f)), Palette::text);
        juce::TextLayout tl; tl.createLayout(as, 300.0f);
        const int w = int(tl.getWidth()) + 24, h = int(tl.getHeight()) + 16;
        return juce::Rectangle<int>(pos.x > parent.getCentreX() ? pos.x - w - 6 : pos.x + 14, pos.y > parent.getCentreY() ? pos.y - h - 8 : pos.y + 20, w, h).constrainedWithin(parent);
    }
    void drawTooltip(juce::Graphics& g, const juce::String& tip, int w, int h) override
    {
        auto r = juce::Rectangle<float>(0, 0, float(w), float(h)).reduced(1.0f);
        g.setColour(Palette::bg2.withAlpha(0.97f)); g.fillRoundedRectangle(r, 9.0f);
        g.setColour(accent.withAlpha(0.8f)); g.drawRoundedRectangle(r, 9.0f, 1.0f);
        juce::AttributedString as; as.append(tip, juce::Font(juce::FontOptions(12.0f)), Palette::text);
        juce::TextLayout tl; tl.createLayout(as, float(w) - 24.0f);
        tl.draw(g, r.reduced(11.0f, 7.0f));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.0f);
        if (bool(b.getProperties()["chip"]))
        {
            const float rad = r.getHeight() * 0.5f;
            if (hover || down) { for (int i = 3; i >= 1; --i) { g.setColour(accent2.withAlpha(0.06f)); g.fillRoundedRectangle(r.expanded(i * 1.6f), rad + i * 1.6f); } }
            g.setColour(accent.withAlpha(down ? 0.45f : (hover ? 0.28f : 0.16f))); g.fillRoundedRectangle(r, rad);
            g.setColour(hover || down ? accent2.withAlpha(0.9f) : accent.withAlpha(0.6f)); g.drawRoundedRectangle(r, rad, 0.8f);
            return;
        }
        if (bool(b.getProperties()["round"]))
        {
            r = r.withSizeKeepingCentre(juce::jmin(r.getWidth(), r.getHeight()), juce::jmin(r.getWidth(), r.getHeight()));
            g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillEllipse(r.translated(0, 2.0f));
            g.setGradientFill(juce::ColourGradient(Palette::knobLight, r.getX(), r.getY(), Palette::card, r.getX(), r.getBottom(), false));
            g.fillEllipse(r);
            if (hover) { for (int i = 3; i >= 1; --i) { g.setColour(accent2.withAlpha(0.07f)); g.drawEllipse(r.expanded(i * 1.6f), 1.0f); } }
            g.setColour(hover || down ? accent2 : accent.withAlpha(0.7f)); g.drawEllipse(r, 1.2f);
            return;
        }
        g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillRoundedRectangle(r.translated(0, 2.0f), 8.0f);
        if (down) g.setGradientFill(juce::ColourGradient(accent, r.getX(), r.getY(), accent2.darker(0.2f), r.getRight(), r.getBottom(), false));
        else g.setGradientFill(juce::ColourGradient(Palette::knobLight, r.getX(), r.getY(), Palette::card, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 8.0f);
        if (hover && ! down) { for (int i = 3; i >= 1; --i) { g.setColour(accent.withAlpha(0.05f)); g.drawRoundedRectangle(r.expanded(i * 1.5f), 8.0f, 1.0f); } }
        g.setColour(hover || down ? accent.withAlpha(0.9f) : Palette::cardEdge); g.drawRoundedRectangle(r, 8.0f, 1.0f);
    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool hover, bool down) override
    {
        if (bool(b.getProperties()["chip"]))
        {
            // small open-book glyph + tracked label
            auto r = b.getLocalBounds().toFloat();
            const float h = r.getHeight(), bx = r.getX() + h * 0.55f, by = r.getCentreY();
            const auto col = hover || down ? juce::Colours::white : accentHi;
            g.setColour(col);
            const bool book = bool(b.getProperties()["book"]);
            if (book)
            {
                juce::Path bk;
                bk.addRoundedRectangle(bx - 5.5f, by - 3.2f, 5.0f, 6.4f, 1.0f);
                bk.addRoundedRectangle(bx + 0.5f, by - 3.2f, 5.0f, 6.4f, 1.0f);
                g.strokePath(bk, juce::PathStrokeType(1.0f));
                g.drawLine(bx, by - 3.2f, bx, by + 3.2f, 1.0f);
            }
            juce::Font f(juce::FontOptions(h * 0.52f, juce::Font::bold)); f.setExtraKerningFactor(0.12f);
            g.setFont(f);
            g.drawText(b.getButtonText(), book ? r.withTrimmedLeft(h * 1.1f).withTrimmedRight(h * 0.35f) : r, juce::Justification::centred);
            return;
        }
        g.setFont(juce::Font(juce::FontOptions(b.getButtonText().length() <= 1 ? 18.0f : 10.5f, juce::Font::bold)));
        g.setColour(down ? juce::Colours::white : (hover ? Palette::text : accentHi));
        g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
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

// Slider with a right-click hook (lock for Warp) and a parameter id for the mod matrix.
class ZwSlider : public juce::Slider
{
public:
    std::function<void()> onContextMenu;
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onContextMenu) onContextMenu(); return; }
        juce::Slider::mouseDown(e);
    }
};

struct Knob
{
    ZwSlider     slider;
    juce::Label  label;
    juce::String paramId;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;

    void init(juce::Component& parent, juce::AudioProcessorValueTreeState& apvts, const char* pid,
              const juce::String& title, bool big, bool textBox = true)
    {
        paramId = pid;
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(textBox ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox, false, big ? 80 : 66, 17);
        slider.setPopupDisplayEnabled(! textBox, false, &parent);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        parent.addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pid, slider);
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

    // Editor timer: feed modulation range + live position and lock state into the look-and-feel
    void setMod(float lo, float hi, float pos) { slider.getProperties().set("modLo", lo); slider.getProperties().set("modHi", hi); slider.getProperties().set("modPos", pos); }
    void clearMod() { slider.getProperties().set("modPos", -1.0f); }
    void setLocked(bool on) { slider.getProperties().set("locked", on); }
};

// ---- Background: static (cached) + animated layers ------------------------------------

inline void drawBackgroundStatic(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour accent, juce::Colour accent2)
{
    g.setGradientFill(juce::ColourGradient(Palette::bg2, b.getCentreX(), b.getY(), Palette::bg, b.getCentreX(), b.getBottom(), false));
    g.fillRect(b);
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.14f), b.getWidth() * 0.85f, b.getHeight() * 0.95f, juce::Colours::transparentBlack, b.getWidth() * 0.45f, b.getHeight() * 0.45f, true));
    g.fillRect(b);
    g.setGradientFill(juce::ColourGradient(accent2.withAlpha(0.07f), b.getWidth() * 0.1f, b.getHeight() * 0.15f, juce::Colours::transparentBlack, b.getWidth() * 0.45f, b.getHeight() * 0.5f, true));
    g.fillRect(b);
    g.setGradientFill(juce::ColourGradient(Palette::magenta.withAlpha(0.04f), b.getWidth() * 0.5f, b.getHeight() * 1.05f, juce::Colours::transparentBlack, b.getWidth() * 0.5f, b.getHeight() * 0.6f, true));
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
    g.setGradientFill(juce::ColourGradient(accent2.withAlpha(0.04f), vx, vy, juce::Colours::transparentBlack, vx + b.getWidth() * 0.45f, vy, true));
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
    g.setColour(Palette::card.withAlpha(0.66f));
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
    // Wordmark: wide-tracked, gradient fill, thin outer glow, hairline underline that fades
    const juce::String spaced = [&] { juce::String o; for (auto ch : title) { o << ch << " "; } return o.trimEnd(); }();
    const auto tr = juce::Rectangle<float>(26 * s, 14 * s, 420 * s, 42 * s);
    juce::Font f(juce::FontOptions(34.0f * s, juce::Font::bold));
    f.setExtraKerningFactor(0.08f);
    g.setFont(f);
    for (int i = 4; i >= 1; --i) { g.setColour(accent.withAlpha(0.07f)); g.drawText(title, tr.expanded(i * 1.2f, 0), juce::Justification::centredLeft); }
    g.setGradientFill(juce::ColourGradient(Palette::text, tr.getX(), tr.getY(), accentHi, tr.getX() + 200 * s, tr.getBottom(), false));
    g.drawText(title, tr, juce::Justification::centredLeft);

    // Measured underline under the word
    const float tw = juce::jmin(260.0f * s, juce::GlyphArrangement::getStringWidth(f, title) * 1.08f);
    juce::Path ul; ul.startNewSubPath(26 * s, 58 * s); ul.lineTo(26 * s + tw, 58 * s);
    g.setGradientFill(juce::ColourGradient(accent2, 26 * s, 58 * s, juce::Colours::transparentBlack, 26 * s + tw, 58 * s, false));
    g.strokePath(ul, juce::PathStrokeType(1.5f * s));
    juce::Path ulGlow; ulGlow.startNewSubPath(26 * s, 58 * s); ulGlow.lineTo(26 * s + tw * 0.45f, 58 * s);
    glowStroke(g, ulGlow, accent2, 1.5f * s, 1.3f);

    // Subtitle chip
    g.setFont(juce::Font(juce::FontOptions(9.5f * s, juce::Font::bold)));
    const float cw = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), subtitle) + 18 * s;
    const auto chip = juce::Rectangle<float>(26 * s, 64 * s, cw, 15 * s);
    g.setColour(accent.withAlpha(0.16f)); g.fillRoundedRectangle(chip, 7.5f * s);
    g.setColour(accent.withAlpha(0.6f));  g.drawRoundedRectangle(chip, 7.5f * s, 0.8f);
    g.setColour(accentHi); g.drawText(subtitle, chip, juce::Justification::centred);

    // Wordmark right
    g.setColour(Palette::dim);
    g.setFont(juce::Font(juce::FontOptions(9.0f * s, juce::Font::bold)));
    g.drawText("Z E N W A V E   L A B S", juce::Rectangle<float>(r.getWidth() - 420 * s, 66 * s, 304 * s, 12 * s), juce::Justification::centredRight);
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

// Editor-side analyser: 4096-point FFT, 160 log bins (20 Hz..20 kHz), fast attack / slow
// release ballistics plus a peak-hold trace that falls slowly, like a mastering analyser.
class SpectrumAnalyser
{
public:
    static constexpr int kOrder = 12, kSize = 1 << kOrder, kBins = 160;
    SpectrumAnalyser() : fft(kOrder), window(kSize, juce::dsp::WindowingFunction<float>::blackmanHarris) { levels.fill(0.0f); peaks.fill(0.0f); }

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
            // +3 dB/oct tilt so pink-ish program material reads flat, like most pro analysers
            const float tilt = 0.5f * std::log2(juce::jmax(f0, 20.0f) / 1000.0f) * 3.0f;
            const float db = juce::Decibels::gainToDecibels(pk / (kSize * 0.18f) + 1e-9f) + tilt;
            const float norm = juce::jlimit(0.0f, 1.0f, (db + 84.0f) / 84.0f);
            auto& L = levels[size_t(b)]; L = norm > L ? L + (norm - L) * 0.55f : L * 0.90f + norm * 0.10f;
            auto& P = peaks[size_t(b)];  P = norm > P ? norm : P - 0.004f;
        }
    }
    static float freqForBin(int b) { return 20.0f * std::pow(1000.0f, float(b) / kBins); }   // 20 Hz .. 20 kHz
    std::array<float, kBins> levels, peaks;
private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, kSize> buf {};
    std::array<float, kSize * 2> fftData {};
};

struct EqCurve { float lowCutHz, highCutHz; int lowSlope, highSlope; float tilt; };

// LEARN: listens to the dry input for a few seconds and finds where the sub fundamental sits and
// where the top end rolls off, so the plugin can place its cuts (and Gravity) around *this* track.
class Learner
{
public:
    struct Result { float subHz = 50.0f, rollOffHz = 16000.0f; bool valid = false; };
    void start(float seconds) { avg.fill(0.0f); frames = 0; target = seconds; elapsed = 0.0f; active = true; }
    bool isActive() const { return active; }
    float progress() const { return active ? juce::jlimit(0.0f, 1.0f, elapsed / target) : 0.0f; }
    // Feed the input analyser's bins each editor tick; returns true on the tick learning completes
    bool feed(const std::array<float, SpectrumAnalyser::kBins>& levels, float dt)
    {
        if (! active) return false;
        for (int b = 0; b < SpectrumAnalyser::kBins; ++b) avg[size_t(b)] += levels[size_t(b)];
        ++frames; elapsed += dt;
        if (elapsed < target) return false;
        active = false;
        for (auto& v : avg) v /= float(juce::jmax(1, frames));
        // sub fundamental: strongest bin between 25 and 140 Hz
        int best = -1; float bestV = 0.0f;
        for (int b = 0; b < SpectrumAnalyser::kBins; ++b)
        {
            const float hz = SpectrumAnalyser::freqForBin(b);
            if (hz < 25.0f || hz > 140.0f) continue;
            if (avg[size_t(b)] > bestV) { bestV = avg[size_t(b)]; best = b; }
        }
        float peak = 0.0f; for (auto v : avg) peak = juce::jmax(peak, v);
        result.valid = peak > 0.15f;      // something was actually playing
        result.subHz = best >= 0 ? SpectrumAnalyser::freqForBin(best) : 50.0f;
        // roll-off: highest bin still within 36 dB (0.43 normalised) of the loudest bin
        result.rollOffHz = 16000.0f;
        for (int b = SpectrumAnalyser::kBins - 1; b >= 0; --b)
            if (avg[size_t(b)] > peak - 36.0f / 84.0f) { result.rollOffHz = SpectrumAnalyser::freqForBin(b); break; }
        return true;
    }
    Result result;
private:
    std::array<float, SpectrumAnalyser::kBins> avg {};
    int frames = 0; float target = 6.0f, elapsed = 0.0f; bool active = false;
};

// The "field": a real-time audio spectrum (wet in colour, dry input as a ghost line, peak-hold
// trace) with the EQ response drawn over it and cutoff handles.
// dB <-> y mapping of the EQ overlay inside drawEqField (±24 dB around a line at 32 % height)
inline float eqFieldYForDb(juce::Rectangle<float> r, float db) { const float top = r.getY() + 6.0f, H = r.getBottom() - 1.0f - top; return top + H * 0.32f - db / 24.0f * H * 0.30f; }
inline float eqFieldDbForY(juce::Rectangle<float> r, float y) { const float top = r.getY() + 6.0f, H = r.getBottom() - 1.0f - top; return (top + H * 0.32f - y) / (H * 0.30f) * 24.0f; }

// customDb: when given, replaces the cut/tilt curve (and hides the cutoff handles) - used by synths with a node EQ
inline void drawEqField(juce::Graphics& g, juce::Rectangle<float> r, const SpectrumAnalyser& wet, const SpectrumAnalyser* dry,
                        const EqCurve& eq, juce::Colour accent, juce::Colour accent2, float pulse,
                        const std::function<float(float)>& customDb = nullptr, const char* legend = "WET")
{
    auto xFor = [&](float hz) { return r.getX() + r.getWidth() * std::log(hz / 20.0f) / std::log(1000.0f); };
    const float top = r.getY() + 6.0f, bottom = r.getBottom() - 1.0f, H = bottom - top;
    auto yForNorm = [&](float n) { return bottom - n * H; };
    auto yForDb = [&](float db) { return top + H * 0.32f - db / 24.0f * H * 0.30f; };   // EQ overlay: ±24 dB around a line at 32 %

    juce::Graphics::ScopedSaveState ss(g);
    juce::Path clip; clip.addRoundedRectangle(r, 10.0f); g.reduceClipRegion(clip);
    g.setColour(Palette::bg.withAlpha(0.7f));
    g.fillRoundedRectangle(r, 10.0f);

    // grid: octave-ish frequency lines + 12 dB level lines
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    for (float hz : { 30.f, 40.f, 50.f, 60.f, 70.f, 80.f, 90.f, 100.f, 200.f, 300.f, 400.f, 500.f, 600.f, 700.f, 800.f, 900.f, 1000.f, 2000.f, 3000.f, 4000.f, 5000.f, 6000.f, 7000.f, 8000.f, 9000.f, 10000.f })
        g.drawVerticalLine(int(xFor(hz)), top, bottom);
    g.setColour(juce::Colours::white.withAlpha(0.09f));
    for (float hz : { 100.f, 1000.f, 10000.f }) g.drawVerticalLine(int(xFor(hz)), top, bottom);
    for (int db = -12; db >= -72; db -= 12) { g.setColour(juce::Colours::white.withAlpha(db % 24 == 0 ? 0.07f : 0.035f)); g.drawHorizontalLine(int(yForNorm((db + 84.0f) / 84.0f)), r.getX(), r.getRight()); }
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.setColour(Palette::dim.withAlpha(0.75f));
    for (auto [hz, t] : std::initializer_list<std::pair<float, const char*>>{ { 50.f, "50" }, { 100.f, "100" }, { 200.f, "200" }, { 500.f, "500" }, { 1000.f, "1k" }, { 2000.f, "2k" }, { 5000.f, "5k" }, { 10000.f, "10k" } })
        g.drawText(t, int(xFor(hz)) + 2, int(bottom) - 11, 26, 10, juce::Justification::left);
    for (int db = -12; db >= -60; db -= 24) g.drawText(juce::String(db), int(r.getRight()) - 22, int(yForNorm((db + 84.0f) / 84.0f)) - 10, 20, 10, juce::Justification::right);

    auto spectrumPath = [&](const std::array<float, SpectrumAnalyser::kBins>& v, bool closed)
    {
        juce::Path p;
        auto pt = [&](int b) { return juce::Point<float>(r.getX() + r.getWidth() * float(b) / (SpectrumAnalyser::kBins - 1), yForNorm(v[size_t(b)])); };
        if (closed) { p.startNewSubPath(r.getX(), bottom); p.lineTo(pt(0)); } else p.startNewSubPath(pt(0));
        for (int b = 1; b < SpectrumAnalyser::kBins - 1; ++b) { const auto a = pt(b), c = pt(b + 1); p.quadraticTo(a, (a + c) * 0.5f); }
        p.lineTo(pt(SpectrumAnalyser::kBins - 1));
        if (closed) { p.lineTo(r.getRight(), bottom); p.closeSubPath(); }
        return p;
    };

    // dry input ghost
    if (dry != nullptr)
    {
        auto gp = spectrumPath(dry->levels, true);
        g.setColour(juce::Colours::white.withAlpha(0.05f)); g.fillPath(gp);
        g.setColour(juce::Colours::white.withAlpha(0.22f)); g.strokePath(spectrumPath(dry->levels, false), juce::PathStrokeType(1.0f));
    }
    // wet spectrum: aurora fill, bright edge, soft glow
    {
        auto fillP = spectrumPath(wet.levels, true);
        g.setGradientFill(juce::ColourGradient(accent2.withAlpha(0.55f + 0.15f * pulse), r.getX(), top, accent.withAlpha(0.06f), r.getX(), bottom, false));
        g.fillPath(fillP);
        // vertical "bars" shimmer: thin lines every 4 bins fading up the fill
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        for (int b = 0; b < SpectrumAnalyser::kBins; b += 4) { const float x = r.getX() + r.getWidth() * float(b) / (SpectrumAnalyser::kBins - 1); g.drawVerticalLine(int(x), yForNorm(wet.levels[size_t(b)]), bottom); }
        auto edge = spectrumPath(wet.levels, false);
        glowStroke(g, edge, accent2.withAlpha(0.9f), 1.4f, 0.8f + pulse * 0.5f);
        // peak hold
        juce::Path pk = spectrumPath(wet.peaks, false);
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.strokePath(pk, juce::PathStrokeType(0.8f));
    }

    // EQ response overlay
    juce::Path curve; bool first = true;
    for (int i = 0; i <= 160; ++i)
    {
        const float hz = 20.0f * std::pow(1000.0f, float(i) / 160.0f);
        const int nLow = 1 << eq.lowSlope, nHigh = 1 << eq.highSlope;                 // order: 1,2,4,8
        const float hpMag = 1.0f / std::sqrt(1.0f + std::pow(eq.lowCutHz / hz, 2.0f * nLow));
        const float lpMag = 1.0f / std::sqrt(1.0f + std::pow(hz / eq.highCutHz, 2.0f * nHigh));
        const float tiltDb = eq.tilt * 6.0f * std::tanh(std::log(hz / 1000.0f) * 0.9f);
        const float db = customDb ? customDb(hz) : juce::Decibels::gainToDecibels(hpMag * lpMag + 1e-6f) + tiltDb;
        const float x = xFor(hz), y = juce::jlimit(top, bottom, yForDb(db));
        if (first) { curve.startNewSubPath(x, y); first = false; } else curve.lineTo(x, y);
    }
    // shaded area under the EQ curve shows what the cuts remove
    {
        juce::Path shade(curve); shade.lineTo(r.getRight(), top); shade.lineTo(r.getX(), top); shade.closeSubPath();
        g.setColour(Palette::magenta.withAlpha(0.06f)); g.fillPath(shade);
    }
    glowStroke(g, curve, juce::Colours::white.interpolatedWith(accent, 0.2f), 1.8f, 0.9f + pulse * 0.5f);

    // cutoff handles
    if (! customDb) for (float hz : { eq.lowCutHz, eq.highCutHz })
    {
        const float x = xFor(juce::jlimit(20.0f, 20000.0f, hz));
        g.setColour(accent.withAlpha(0.5f));
        g.drawVerticalLine(int(x), top, bottom);
        const float y = yForDb(-3.0f);
        g.setColour(accent2.withAlpha(0.4f)); g.fillEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colours::white); g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }
    // legend
    g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
    g.setColour(accent2); g.drawText(legend, int(r.getX()) + 8, int(top) + 2, 30, 10, juce::Justification::left);
    if (dry) { g.setColour(juce::Colours::white.withAlpha(0.45f)); g.drawText("IN", int(r.getX()) + 34, int(top) + 2, 20, 10, juce::Justification::left); }
    g.setColour(accent.withAlpha(0.5f));
    g.drawRoundedRectangle(r, 10.0f, 1.0f);
}

} // namespace zwui

namespace zwui {

// Galaxy scope: a vectorscope with persistence. Mid = vertical axis, Side = horizontal.
// Wide material blooms into a nebula; mono collapses to a beam. A correlation ring
// around the outside turns from accent2 (in phase) to magenta (out of phase).
class GalaxyScope
{
public:
    template <class Tap> void update(const Tap& tap)
    {
        tap.snapshot(L.data(), R.data());
        double sLR = 0, sLL = 0, sRR = 0;
        for (int i = 0; i < Tap::kPairs; ++i) { sLR += L[size_t(i)] * R[size_t(i)]; sLL += L[size_t(i)] * L[size_t(i)]; sRR += R[size_t(i)] * R[size_t(i)]; }
        const double den = std::sqrt(sLL * sRR) + 1e-9;
        const float corrNow = float(sLR / den);
        correlation += (corrNow - correlation) * 0.15f;
        level = float(std::sqrt((sLL + sRR) / (2.0 * Tap::kPairs)));
    }

    void draw(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, juce::Colour accent2, float pulse)
    {
        const int w = int(r.getWidth()), h = int(r.getHeight());
        if (w < 8 || h < 8) return;
        if (persist.getWidth() != w || persist.getHeight() != h)
            persist = juce::Image(juce::Image::ARGB, w, h, true);

        // fade the persistence layer
        {
            juce::Graphics pg(persist);
            pg.setColour(juce::Colours::black.withAlpha(0.22f));
            pg.fillAll();   // ARGB with black alpha → darkens, keeps glow trails
            // draw new points
            const auto c = juce::Point<float>(w * 0.5f, h * 0.5f);
            const float scale = juce::jmin(w, h) * 0.42f;
            const float gain = level > 1e-4f ? juce::jlimit(1.0f, 6.0f, 0.35f / level) : 1.0f;  // auto-gain so quiet material still shows
            for (int i = 0; i < 1024; i += 2)
            {
                const float l = L[size_t(i)] * gain, rr = R[size_t(i)] * gain;
                const float mid = (l + rr) * 0.7071f, side = (l - rr) * 0.7071f;
                const float x = c.x + juce::jlimit(-1.0f, 1.0f, side) * scale;
                const float y = c.y - juce::jlimit(-1.0f, 1.0f, mid) * scale;
                const float e = juce::jlimit(0.0f, 1.0f, std::sqrt(l * l + rr * rr));
                pg.setColour(aurora(e, accent, accent2).withAlpha(0.35f + 0.5f * e));
                pg.fillEllipse(x - 1.2f, y - 1.2f, 2.4f, 2.4f);
                if ((i & 6) == 0) { pg.setColour(juce::Colours::white.withAlpha(0.25f * e)); pg.fillEllipse(x - 0.6f, y - 0.6f, 1.2f, 1.2f); }
            }
        }

        // backdrop rings + axes
        const auto c = r.getCentre();
        const float R0 = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;
        g.setColour(Palette::bg.withAlpha(0.6f));
        g.fillEllipse(c.x - R0, c.y - R0, R0 * 2, R0 * 2);
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        for (float k : { 0.33f, 0.66f }) g.drawEllipse(c.x - R0 * k, c.y - R0 * k, R0 * 2 * k, R0 * 2 * k, 1.0f);
        g.drawLine(c.x - R0, c.y, c.x + R0, c.y, 1.0f);
        g.drawLine(c.x, c.y - R0, c.x, c.y + R0, 1.0f);
        // L / R diagonals
        g.setColour(accent.withAlpha(0.18f));
        g.drawLine(c.x - R0 * 0.707f, c.y - R0 * 0.707f, c.x + R0 * 0.707f, c.y + R0 * 0.707f, 1.0f);
        g.drawLine(c.x + R0 * 0.707f, c.y - R0 * 0.707f, c.x - R0 * 0.707f, c.y + R0 * 0.707f, 1.0f);

        // persistence image, clipped to the circle, with a soft glow pass
        {
            juce::Graphics::ScopedSaveState ss(g);
            juce::Path clip; clip.addEllipse(c.x - R0 + 2, c.y - R0 + 2, R0 * 2 - 4, R0 * 2 - 4);
            g.reduceClipRegion(clip);
            g.setOpacity(0.55f);
            g.drawImage(persist, r.expanded(3.0f), juce::RectanglePlacement::stretchToFit);  // slightly enlarged = glow
            g.setOpacity(1.0f);
            g.drawImageAt(persist, int(r.getX()), int(r.getY()));
        }

        // correlation ring: arc from top, length = |corr|, colour by sign
        const float corr = juce::jlimit(-1.0f, 1.0f, correlation);
        juce::Path ring; ring.addCentredArc(c.x, c.y, R0 - 2.0f, R0 - 2.0f, 0.0f, -juce::MathConstants<float>::pi * std::abs(corr), juce::MathConstants<float>::pi * std::abs(corr), true);
        const auto ringCol = corr >= 0 ? accent2 : Palette::magenta;
        glowStroke(g, ring, ringCol.withAlpha(0.9f), 2.0f, 0.8f + pulse * 0.6f);
        g.setColour(Palette::cardEdge);
        g.drawEllipse(c.x - R0, c.y - R0, R0 * 2, R0 * 2, 1.0f);

        g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
        g.setColour(Palette::dim);
        g.drawText("L", int(c.x - R0 * 0.85f), int(c.y - R0 * 0.95f), 12, 10, juce::Justification::centred);
        g.drawText("R", int(c.x + R0 * 0.85f) - 12, int(c.y - R0 * 0.95f), 12, 10, juce::Justification::centred);
        g.setColour(ringCol);
        g.drawText(juce::String(corr, 2), r.toNearestInt().removeFromBottom(12), juce::Justification::centred);
    }

private:
    std::array<float, 1024> L {}, R {};
    juce::Image persist;
    float correlation = 1.0f, level = 0.0f;
};

} // namespace zwui
