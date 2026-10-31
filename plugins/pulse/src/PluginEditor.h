#pragma once
#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include <zenwave_ui/ModPanel.h>
#include <zenwave_ui/PresetBar.h>
#include "Manual.h"
#include "PluginProcessor.h"

// Spectrum overlay: draggable band splits + the four dynamic-EQ nodes (drag = freq/range,
// shift-drag = width, double-click = on/off) + live gain-reduction shading per band.
class PulseOverlay : public juce::Component, public juce::SettableTooltipClient
{
public:
    PulseOverlay(PulseAudioProcessor& p, zwui::LookAndFeel& l) : proc(p), lnf(l)
    {
        pLow = p.apvts.getParameter(ParamIDs::lowX); pHigh = p.apvts.getParameter(ParamIDs::highX);
        for (int i = 0; i < 4; ++i) { nOn[i] = p.apvts.getParameter(ParamIDs::nodeOn(i)); nF[i] = p.apvts.getParameter(ParamIDs::nodeFreq(i)); nQ[i] = p.apvts.getParameter(ParamIDs::nodeQ(i)); nR[i] = p.apvts.getParameter(ParamIDs::nodeRange(i)); }
        setTooltip("Drag the two lines to move the band splits. Dynamic EQ nodes: drag = frequency / range, Shift-drag = width, double-click = on/off. Shaded areas show live gain reduction.");
    }
    static float xFor(float hz, juce::Rectangle<float> r) { return r.getX() + r.getWidth() * std::log(hz / 20.0f) / std::log(1000.0f); }
    static float hzFor(float x, juce::Rectangle<float> r) { return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
    float plain(juce::RangedAudioParameter* p) const { return p->convertFrom0to1(p->getValue()); }
    float yForDb(float db, juce::Rectangle<float> r) const { return r.getCentreY() - db / 18.0f * r.getHeight() * 0.42f; }

    void paint(juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        const float xl = xFor(plain(pLow), r), xh = xFor(plain(pHigh), r);
        // live gain reduction per band: shaded from the top, depth = dB (0..-18)
        const float edges[4] = { r.getX(), xl, xh, r.getRight() };
        for (int b = 0; b < 3; ++b)
        {
            const float gr = proc.bandGr[size_t(b)].load();
            const float h = juce::jlimit(0.0f, 1.0f, -gr / 18.0f) * r.getHeight() * 0.5f;
            if (h > 0.5f) { g.setGradientFill(juce::ColourGradient(lnf.accent2.withAlpha(0.35f), 0, r.getY(), lnf.accent2.withAlpha(0.02f), 0, r.getY() + h, false)); g.fillRect(juce::Rectangle<float>(edges[b], r.getY(), edges[b + 1] - edges[b], h)); }
            const float up = juce::jlimit(0.0f, 1.0f, gr / 18.0f) * r.getHeight() * 0.5f;   // upward: from the bottom
            if (up > 0.5f) { g.setGradientFill(juce::ColourGradient(lnf.accent.withAlpha(0.30f), 0, r.getBottom(), lnf.accent.withAlpha(0.02f), 0, r.getBottom() - up, false)); g.fillRect(juce::Rectangle<float>(edges[b], r.getBottom() - up, edges[b + 1] - edges[b], up)); }
            g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold))); g.setColour(zwui::Palette::dim.withAlpha(0.9f));
            g.drawText(juce::String(gr, 1), int(edges[b]), int(r.getY()) + 3, int(edges[b + 1] - edges[b]), 10, juce::Justification::centred);
        }
        // dividers
        for (int i = 0; i < 2; ++i)
        {
            const float x = i == 0 ? xl : xh; const bool hot = drag == i || hover == i;
            juce::Path p; p.startNewSubPath(x, r.getY() + 2.0f); p.lineTo(x, r.getBottom() - 2.0f);
            zwui::glowStroke(g, p, (hot ? juce::Colours::white : lnf.accent2).withAlpha(0.8f), hot ? 2.0f : 1.2f, 0.7f);
            g.setColour(hot ? juce::Colours::white : lnf.accent2); g.fillRoundedRectangle(x - 2.5f, r.getBottom() - 22.0f, 5.0f, 14.0f, 2.5f);
        }
        // nodes: bell of the live gain + handle at (freq, range)
        for (int i = 0; i < 4; ++i)
        {
            const bool on = nOn[i]->getValue() > 0.5f;
            const float f = plain(nF[i]), q = plain(nQ[i]), range = plain(nR[i]), live = proc.nodeGain[size_t(i)].load();
            const float x = xFor(f, r);
            const bool hot = drag == 10 + i || hover == 10 + i;
            if (on)
            {
                juce::Path bell; const float bw = r.getWidth() * 0.9f / std::sqrt(q) * 0.12f;
                for (int k = 0; k <= 40; ++k) { const float px = x - bw * 2 + bw * 4 * k / 40.0f; const float d = (px - x) / bw; const float y = yForDb(live * std::exp(-d * d * 2.0f), r); if (k == 0) bell.startNewSubPath(px, y); else bell.lineTo(px, y); }
                g.setColour((range < 0 ? zwui::Palette::magenta : lnf.accent2).withAlpha(0.9f)); g.strokePath(bell, juce::PathStrokeType(1.4f));
                juce::Path fill(bell); fill.lineTo(x + bw * 2, r.getCentreY()); fill.lineTo(x - bw * 2, r.getCentreY()); fill.closeSubPath();
                g.setColour((range < 0 ? zwui::Palette::magenta : lnf.accent2).withAlpha(0.15f)); g.fillPath(fill);
            }
            const float y = yForDb(range, r);
            if (! on && ! hot) { g.setColour(zwui::Palette::dim.withAlpha(0.5f)); g.drawEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f, 1.0f); continue; }
            g.setColour((on ? (range < 0 ? zwui::Palette::magenta : lnf.accent2) : zwui::Palette::dim).withAlpha(hot ? 1.0f : 0.85f));
            g.fillEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f);
            g.setColour(zwui::Palette::bg); g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
            g.drawText(juce::String(i + 1), int(x) - 6, int(y) - 6, 12, 12, juce::Justification::centred);
            if (hot) { g.setColour(juce::Colours::white); g.drawText((f >= 1000 ? juce::String(f / 1000, 1) + "k" : juce::String(int(f))) + "  " + juce::String(range, 1) + " dB  Q" + juce::String(q, 1), int(x) + 9, int(y) - 7, 110, 12, juce::Justification::left); }
        }
        g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawHorizontalLine(int(r.getCentreY()), r.getX(), r.getRight());
    }
    int hit(juce::Point<float> p) const
    {
        const auto r = getLocalBounds().toFloat();
        for (int i = 0; i < 4; ++i) { const auto c = juce::Point<float>(xFor(plain(nF[i]), r), yForDb(plain(nR[i]), r)); if (c.getDistanceFrom(p) < 9.0f) return 10 + i; }
        const float dl = std::abs(p.x - xFor(plain(pLow), r)), dh = std::abs(p.x - xFor(plain(pHigh), r));
        if (juce::jmin(dl, dh) <= 10.0f) return dl <= dh ? 0 : 1;
        return -1;
    }
    bool hitTest(int x, int y) override { return hit({ float(x), float(y) }) >= 0; }
    void mouseMove(const juce::MouseEvent& e) override { hover = hit(e.position); setMouseCursor(hover >= 10 ? juce::MouseCursor::DraggingHandCursor : hover >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor); repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown(const juce::MouseEvent& e) override
    {
        drag = hit(e.position);
        if (drag < 0) return;
        if (drag >= 10) { const int i = drag - 10; nF[i]->beginChangeGesture(); nR[i]->beginChangeGesture(); nQ[i]->beginChangeGesture(); startQ = plain(nQ[i]); }
        else (drag == 0 ? pLow : pHigh)->beginChangeGesture();
    }
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        const int h = hit(e.position);
        if (h >= 10) { auto* p = nOn[h - 10]; p->beginChangeGesture(); p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f); p->endChangeGesture(); }
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (drag < 0) return;
        const auto r = getLocalBounds().toFloat();
        if (drag >= 10)
        {
            const int i = drag - 10;
            if (e.mods.isShiftDown()) { const float q = juce::jlimit(0.3f, 12.0f, startQ * std::pow(2.0f, -e.getDistanceFromDragStartY() / 40.0f)); nQ[i]->setValueNotifyingHost(nQ[i]->convertTo0to1(q)); }
            else
            {
                nF[i]->setValueNotifyingHost(nF[i]->convertTo0to1(hzFor(e.position.x, r)));
                const float db = juce::jlimit(-18.0f, 18.0f, (r.getCentreY() - e.position.y) / (r.getHeight() * 0.42f) * 18.0f);
                nR[i]->setValueNotifyingHost(nR[i]->convertTo0to1(db));
            }
        }
        else { auto* p = drag == 0 ? pLow : pHigh; p->setValueNotifyingHost(p->convertTo0to1(hzFor(e.position.x, r))); }
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        if (drag >= 10) { const int i = drag - 10; nF[i]->endChangeGesture(); nR[i]->endChangeGesture(); nQ[i]->endChangeGesture(); }
        else if (drag >= 0) (drag == 0 ? pLow : pHigh)->endChangeGesture();
        drag = -1;
    }
private:
    PulseAudioProcessor& proc; zwui::LookAndFeel& lnf;
    juce::RangedAudioParameter* pLow{}, * pHigh{}, * nOn[4]{}, * nF[4]{}, * nQ[4]{}, * nR[4]{};
    int drag = -1, hover = -1; float startQ = 2.0f;
};

class PulseAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PulseAudioProcessorEditor(PulseAudioProcessor&);
    ~PulseAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void installLocks();
    void syncModRings();
    void applyMatch();

    PulseAudioProcessor& processor;
    // Pulse: electric teal secondary - control, not heat
    zwui::LookAndFeel lnf { juce::Colour(0xff7c5cff), juce::Colour(0xffd6ccff), juce::Colour(0xff2ee6c5) };
    zwui::Stage stage;

    zwui::Knob depth, time, shapeAtk, shapeSus, mix, output;        // macro row
    zwui::Knob input, duck, scAmount, scRelease, lowX, highX;       // row 2
    juce::ToggleButton autoButton { "AUTO" }, subLockButton { "SUB LOCK" }, gritButton { "GRIT" }, powerButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAttach, subLockAttach, gritAttach, powerAttach;
    zwui::Knob bandDepth[3], bandGain[3], bandAtk[3], bandRel[3];
    juce::ToggleButton bandOn[3], bandSolo[3];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandOnAttach[3], bandSoloAttach[3];
    zwui::Knob nodeFreq[4], nodeQ[4], nodeThr[4], nodeRange[4];
    juce::ToggleButton nodeOn[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> nodeOnAttach[4];
    zwui::SpectrumAnalyser analyser, analyserIn;
    zwui::GalaxyScope scope;
    zwui::ModPanel modPanel { processor.apvts, processor.mod, lnf };
    zwui::PresetBar presetBar;
    zwui::PerformPad pad { processor.apvts, processor.pad, lnf, "DEPTH", "DUCK" };
    juce::ComboBox glideBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> glideAttach;
    PulseOverlay overlay { processor, lnf };
    double lastTick = 0.0;
    std::vector<zwui::Knob*> allKnobs;
    juce::TextButton helpButton { "MANUAL" }, matchButton { "MATCH" };
    zwui::Learner learner;
    double matchIn = 0.0, matchOut = 0.0; int matchFrames = 0;
    juce::TooltipWindow tooltips { this, 650 };
    zwui::ManualOverlay manual { lnf };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseAudioProcessorEditor)
};
