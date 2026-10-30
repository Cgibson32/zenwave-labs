#pragma once
#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include <zenwave_ui/ModPanel.h>
#include <zenwave_ui/PresetBar.h>
#include "Manual.h"
#include "PluginProcessor.h"

// Draggable crossover dividers drawn over the spectrum (same 20 Hz..20 kHz log mapping)
class BandDividers : public juce::Component, public juce::SettableTooltipClient
{
public:
    BandDividers(juce::AudioProcessorValueTreeState& s, zwui::LookAndFeel& l) : lnf(l)
    {
        pLow = s.getParameter(ParamIDs::lowX); pHigh = s.getParameter(ParamIDs::highX);
        setTooltip("Band split points. Drag the lines to move the LOW | MID | HIGH crossovers.");
    }
    static float xFor(float hz, juce::Rectangle<float> r) { return r.getX() + r.getWidth() * std::log(hz / 20.0f) / std::log(1000.0f); }
    static float hzFor(float x, juce::Rectangle<float> r) { return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
    float lowHz() const { return pLow->convertFrom0to1(pLow->getValue()); }
    float highHz() const { return pHigh->convertFrom0to1(pHigh->getValue()); }

    void paint(juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        const float xl = xFor(lowHz(), r), xh = xFor(highHz(), r);
        // band tints
        g.setColour(lnf.accent.withAlpha(0.05f));  g.fillRect(juce::Rectangle<float>(r.getX(), r.getY(), xl - r.getX(), r.getHeight()));
        g.setColour(lnf.accent2.withAlpha(0.04f)); g.fillRect(juce::Rectangle<float>(xh, r.getY(), r.getRight() - xh, r.getHeight()));
        for (int i = 0; i < 2; ++i)
        {
            const float x = i == 0 ? xl : xh; const bool hot = drag == i || hover == i;
            juce::Path p; p.startNewSubPath(x, r.getY() + 2.0f); p.lineTo(x, r.getBottom() - 2.0f);
            zwui::glowStroke(g, p, (hot ? juce::Colours::white : lnf.accent2).withAlpha(0.9f), hot ? 2.0f : 1.4f, 0.8f);
            // grip
            g.setColour(hot ? juce::Colours::white : lnf.accent2);
            g.fillRoundedRectangle(x - 3.0f, r.getCentreY() - 9.0f, 6.0f, 18.0f, 3.0f);
            g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
            const float hz = i == 0 ? lowHz() : highHz();
            const auto txt = hz >= 1000.0f ? juce::String(hz / 1000.0f, 1) + "k" : juce::String(juce::roundToInt(hz));
            g.setColour(zwui::Palette::bg.withAlpha(0.8f)); g.fillRoundedRectangle(x + 5.0f, r.getY() + 14.0f, 30.0f, 11.0f, 4.0f);
            g.setColour(hot ? juce::Colours::white : lnf.accentHi); g.drawText(txt, int(x) + 5, int(r.getY()) + 14, 30, 11, juce::Justification::centred);
        }
        g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
        g.setColour(zwui::Palette::dim.withAlpha(0.8f));
        g.drawText("LOW", int(r.getX()), int(r.getBottom()) - 24, int(xl - r.getX()), 10, juce::Justification::centred);
        g.drawText("MID", int(xl), int(r.getBottom()) - 24, int(xh - xl), 10, juce::Justification::centred);
        g.drawText("HIGH", int(xh), int(r.getBottom()) - 24, int(r.getRight() - xh), 10, juce::Justification::centred);
    }
    int nearest(float x) const
    {
        const auto r = getLocalBounds().toFloat();
        const float dl = std::abs(x - xFor(lowHz(), r)), dh = std::abs(x - xFor(highHz(), r));
        if (juce::jmin(dl, dh) > 14.0f) return -1;
        return dl <= dh ? 0 : 1;
    }
    bool hitTest(int x, int) override { return nearest(float(x)) >= 0; }
    void mouseMove(const juce::MouseEvent& e) override { hover = nearest(e.position.x); setMouseCursor(hover >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor); repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown(const juce::MouseEvent& e) override { drag = nearest(e.position.x); if (drag >= 0) (drag == 0 ? pLow : pHigh)->beginChangeGesture(); }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (drag < 0) return;
        auto* p = drag == 0 ? pLow : pHigh;
        p->setValueNotifyingHost(p->convertTo0to1(hzFor(e.position.x, getLocalBounds().toFloat())));
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { if (drag >= 0) (drag == 0 ? pLow : pHigh)->endChangeGesture(); drag = -1; }
private:
    juce::RangedAudioParameter* pLow{}, * pHigh{}; zwui::LookAndFeel& lnf; int drag = -1, hover = -1;
};

class CoreAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CoreAudioProcessorEditor(CoreAudioProcessor&);
    ~CoreAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void installLocks();
    void syncModRings();
    void applyLearn();

    CoreAudioProcessor& processor;
    // Core's accent: hot violet with an ember-orange secondary (heat)
    zwui::LookAndFeel lnf { juce::Colour(0xff8b5cf6), juce::Colour(0xffd8c7ff), juce::Colour(0xffff7a3d) };
    zwui::Stage stage;

    zwui::Knob drive, heat, focus, width, mix;                 // macro row
    zwui::Knob bandDrive[3], bandMix[3], bandGain[3];          // bands
    juce::ComboBox bandChar[3];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bandCharAttach[3];
    zwui::Knob lowCut, highCut, tilt, output;
    juce::ComboBox lowSlopeBox, highSlopeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowSlopeAttach, highSlopeAttach;
    zwui::SpectrumAnalyser analyser, analyserIn;
    zwui::GalaxyScope scope;
    zwui::ModPanel modPanel { processor.apvts, processor.mod, lnf };
    zwui::PresetBar presetBar;
    zwui::PerformPad pad { processor.apvts, processor.pad, lnf, "DRIVE", "MIX" };
    juce::ComboBox glideBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> glideAttach;
    BandDividers dividers { processor.apvts, lnf };

    juce::ToggleButton subLockButton { "SUB LOCK" }, powerButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> subLockAttach, powerAttach;
    double lastTick = 0.0;
    std::vector<zwui::Knob*> allKnobs;
    juce::TextButton helpButton { "MANUAL" }, learnButton { "LEARN" };
    zwui::Learner learner;
    juce::TooltipWindow tooltips { this, 650 };
    zwui::ManualOverlay manual { lnf };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoreAudioProcessorEditor)
};
