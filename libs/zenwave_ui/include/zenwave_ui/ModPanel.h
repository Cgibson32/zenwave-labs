#pragma once
// MOTION card: two LFOs (shape / sync / division / rate, each with a live waveform scope),
// envelope follower (attack / release / sensitivity, live meter) and four routing slots.
// Designed for an 836 x 170 card at 1x; everything scales with the editor.

#include "ZenWaveUI.h"
#include "Modulation.h"

namespace zwui {

class ModPanel : public juce::Component
{
public:
    ModPanel(juce::AudioProcessorValueTreeState& apvts, ModMatrix& m, LookAndFeel& l) : mod(m), lnf(l)
    {
        for (int i = 0; i < 2; ++i)
        {
            auto& L = lfo[i];
            addAndMakeVisible(L.shape); L.shape.addItemList(ModIDs::shapeNames(), 1);
            L.shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ModIDs::lfoShape(i), L.shape);
            addAndMakeVisible(L.sync);
            L.syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, ModIDs::lfoSync(i), L.sync);
            addAndMakeVisible(L.div); L.div.addItemList(divisionNames(), 1);
            L.divAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ModIDs::lfoDiv(i), L.div);
            L.rate.init(*this, apvts, ModIDs::lfoRate(i).toRawUTF8(), "RATE", false, false);
        }
        attack.init(*this, apvts, ModIDs::envAttack, "ATTACK", false, false);
        release.init(*this, apvts, ModIDs::envRelease, "RELEASE", false, false);
        sens.init(*this, apvts, ModIDs::envSens, "SENS", false, false);
        for (int i = 0; i < ModIDs::kSlots; ++i)
        {
            auto& S = slot[i];
            addAndMakeVisible(S.src); S.src.addItemList(ModIDs::sourceNames(), 1);
            S.srcAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ModIDs::slotSrc(i), S.src);
            addAndMakeVisible(S.dst);
            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(ModIDs::slotDst(i)))) S.dst.addItemList(cp->choices, 1);
            S.dstAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ModIDs::slotDst(i), S.dst);
            S.amt.init(*this, apvts, ModIDs::slotAmt(i).toRawUTF8(), "", false, false);
            S.amt.label.setVisible(false);
        }
        setInterceptsMouseClicks(false, true);
    }

    // scale = editor width / 880
    void layoutScaled(float s)
    {
        auto S = [s](float v) { return juce::roundToInt(v * s); };
        for (int i = 0; i < 2; ++i)
        {
            auto& L = lfo[i]; const int x0 = S(18 + i * 212);
            L.shape.setBounds(x0, S(36), S(86), S(20));
            L.sync.setBounds(x0 + S(90), S(36), S(46), S(20));
            L.div.setBounds(x0 + S(140), S(36), S(58), S(20));
            L.rate.layout(x0 + S(132), S(76), S(66), S(66), S(11));
            L.scope = juce::Rectangle<float>(float(x0), S(62.0f), S(120.0f), S(86.0f));
        }
        const int ex = S(448);
        attack.layout (ex,          S(78), S(60), S(66), S(11));
        release.layout(ex + S(62),  S(78), S(60), S(66), S(11));
        sens.layout   (ex + S(124), S(78), S(60), S(66), S(11));
        envMeter = juce::Rectangle<float>(float(ex), S(38.0f), S(184.0f), S(16.0f));
        for (int i = 0; i < ModIDs::kSlots; ++i)
        {
            auto& Sl = slot[i]; const int y = S(32 + i * 31);
            Sl.src.setBounds(S(634), y, S(64), S(22));
            Sl.dst.setBounds(S(702), y, S(84), S(22));
            Sl.amt.layout(S(788), y - S(7), S(36), S(36), 0);
        }
    }

    void tick()
    {
        for (int i = 0; i < 2; ++i)
        {
            const bool sync = lfo[i].sync.getToggleState();
            lfo[i].div.setEnabled(sync); lfo[i].div.setAlpha(sync ? 1.0f : 0.4f);
            lfo[i].rate.slider.setAlpha(sync ? 0.35f : 1.0f);
        }
        for (auto& Sl : slot) { const bool on = Sl.src.getSelectedId() > 1 && Sl.dst.getSelectedId() > 1; Sl.amt.slider.setAlpha(on ? 1.0f : 0.4f); }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const float s = float(getWidth()) / 836.0f;
        g.setFont(juce::Font(juce::FontOptions(9.0f * s, juce::Font::bold)));
        for (int i = 0; i < 2; ++i)
        {
            const auto& L = lfo[i];
            g.setColour(lnf.accentHi); g.drawText("LFO " + juce::String(i + 1), L.scope.withY(18.0f * s).withHeight(14.0f * s), juce::Justification::centredLeft);
            drawLfoScope(g, L.scope, i, s);
        }
        g.setColour(lnf.accentHi); g.drawText("ENV FOLLOW", envMeter.withY(18.0f * s).withHeight(14.0f * s), juce::Justification::centredLeft);
        drawEnvMeter(g, envMeter, s);
        g.setColour(lnf.accentHi); g.drawText("ROUTING", juce::Rectangle<float>(634.0f * s, 18.0f * s, 150.0f * s, 14.0f * s), juce::Justification::centredLeft);
        // slot connector lines: src → dst → amount, lit when active
        for (int i = 0; i < ModIDs::kSlots; ++i)
        {
            const auto& Sl = slot[i];
            const bool on = Sl.src.getSelectedId() > 1 && Sl.dst.getSelectedId() > 1 && std::abs(Sl.amt.slider.getValue()) > 0.001;
            const float y = Sl.src.getBounds().toFloat().getCentreY();
            g.setColour(on ? lnf.accent2.withAlpha(0.7f) : Palette::dim.withAlpha(0.25f));
            g.drawLine(float(Sl.src.getRight()), y, float(Sl.dst.getX()), y, 1.0f);
            g.drawLine(float(Sl.dst.getRight()), y, float(Sl.amt.slider.getX()) + 4.0f * s, y, 1.0f);
            if (on)
            {
                const int si = Sl.src.getSelectedId() - 2;
                const float v = si < 2 ? (mod.vis[size_t(si)].load() * 0.5f + 0.5f) : mod.vis[2].load();
                const float px = float(Sl.src.getRight()) + (float(Sl.dst.getX()) - float(Sl.src.getRight())) * v;
                g.setColour(juce::Colours::white); g.fillEllipse(px - 2.0f, y - 2.0f, 4.0f, 4.0f);
            }
        }
    }

private:
    void drawLfoScope(juce::Graphics& g, juce::Rectangle<float> r, int i, float s)
    {
        g.setColour(Palette::bg.withAlpha(0.7f)); g.fillRoundedRectangle(r, 8.0f * s);
        g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawHorizontalLine(int(r.getCentreY()), r.getX(), r.getRight());
        const int shape = lfo[i].shape.getSelectedId() - 1;
        const float ph = mod.lfoPhase(i);
        juce::Path p;
        const int N = 96;
        juce::Random rr(77 + i);
        float rndA = rr.nextFloat() * 2 - 1, rndB = rr.nextFloat() * 2 - 1;
        for (int k = 0; k <= N; ++k)
        {
            const float t = float(k) / N;                       // one full cycle
            float v;
            switch (shape)
            {
                case 0: v = std::sin(t * juce::MathConstants<float>::twoPi); break;
                case 1: v = 1.0f - 4.0f * std::abs(t - 0.5f); break;
                case 2: v = 2.0f * t - 1.0f; break;
                case 3: v = 1.0f - 2.0f * t; break;
                case 4: v = t < 0.5f ? 1.0f : -1.0f; break;
                case 5: v = t < 0.5f ? rndA : rndB; break;
                default: { const float u = t < 0.5f ? t * 2 : (t - 0.5f) * 2; const float a = t < 0.5f ? rndA : rndB, b = t < 0.5f ? rndB : rndA; const float e = u * u * (3 - 2 * u); v = a + (b - a) * e; } break;
            }
            const float x = r.getX() + 6.0f * s + (r.getWidth() - 12.0f * s) * t;
            const float y = r.getCentreY() - v * (r.getHeight() * 0.36f);
            if (k == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        glowStroke(g, p, lnf.accent2.withAlpha(0.85f), 1.4f * s, 0.6f + lnf.pulse * 0.4f);
        // phase cursor
        const float cx = r.getX() + 6.0f * s + (r.getWidth() - 12.0f * s) * ph;
        g.setColour(juce::Colours::white.withAlpha(0.18f)); g.drawVerticalLine(int(cx), r.getY() + 4.0f * s, r.getBottom() - 4.0f * s);
        const float cv = mod.vis[size_t(i)].load();
        const float cy = r.getCentreY() - cv * (r.getHeight() * 0.36f);
        g.setColour(lnf.accent2.withAlpha(0.35f)); g.fillEllipse(cx - 6.0f * s, cy - 6.0f * s, 12.0f * s, 12.0f * s);
        g.setColour(juce::Colours::white); g.fillEllipse(cx - 2.5f * s, cy - 2.5f * s, 5.0f * s, 5.0f * s);
        g.setColour(lnf.accent.withAlpha(0.5f)); g.drawRoundedRectangle(r, 8.0f * s, 1.0f);
    }

    void drawEnvMeter(juce::Graphics& g, juce::Rectangle<float> r, float s)
    {
        g.setColour(Palette::bg.withAlpha(0.7f)); g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
        const float v = mod.vis[2].load();
        auto fill = r.reduced(2.0f * s).withWidth((r.getWidth() - 4.0f * s) * v);
        g.setGradientFill(juce::ColourGradient(lnf.accent, r.getX(), r.getY(), lnf.accent2, r.getRight(), r.getY(), false));
        g.fillRoundedRectangle(fill, fill.getHeight() * 0.5f);
        g.setColour(juce::Colours::white.withAlpha(0.6f)); g.fillEllipse(fill.getRight() - 3.0f * s, r.getCentreY() - 3.0f * s, 6.0f * s, 6.0f * s);
        g.setColour(lnf.accent.withAlpha(0.5f)); g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
    }

    struct LfoUI { juce::ComboBox shape, div; juce::ToggleButton sync { "SYNC" }; Knob rate; juce::Rectangle<float> scope;
                   std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach, divAttach;
                   std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach; };
    struct SlotUI { juce::ComboBox src, dst; Knob amt; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAttach, dstAttach; };
    LfoUI lfo[2]; SlotUI slot[ModIDs::kSlots];
    Knob attack, release, sens;
    juce::Rectangle<float> envMeter;
    ModMatrix& mod; LookAndFeel& lnf;
};

} // namespace zwui
