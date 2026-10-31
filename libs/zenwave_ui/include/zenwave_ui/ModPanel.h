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
    ModPanel(juce::AudioProcessorValueTreeState& apvts, ModMatrix& m, LookAndFeel& l) : mod(m), lnf(l), state(apvts)
    {
        for (int l = 0; l < 2; ++l)
        {
            for (int i = 0; i < ModIDs::kSteps; ++i) lfo[l].stepParam[i] = apvts.getParameter(ModIDs::step(l, i));
            lfo[l].glide.init(*this, apvts, ModIDs::glide(l).toRawUTF8(), "GLIDE", false, false);
            lfo[l].glide.slider.setTooltip("Step glide: how much each step slides into the next (0 = hard steps, 100 % = fully smoothed)");
        }
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
        addAndMakeVisible(envSrc);
        envSrc.setTooltip("SC: the envelope listens to the sidechain input instead of this plugin's own input. Route a kick or any other channel to the sidechain in your DAW and it can drive any knob here (and Umbra's DUCK).");
        envSrcAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, ModIDs::envSource, envSrc);
        attack.init(*this, apvts, ModIDs::envAttack, "ATK", false, false);
        decay.init(*this, apvts, ModIDs::envDecay, "DEC", false, false);
        sustain.init(*this, apvts, ModIDs::envSustain, "SUS", false, false);
        release.init(*this, apvts, ModIDs::envRelease, "REL", false, false);
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
        for (int i = 0; i < 2; ++i)
        {
            lfo[i].shape.setTooltip("LFO shape: Sine, Triangle, Saw Up, Saw Down, Square, Random (stepped), Drift (smooth random), Steps (draw this LFO's own 16-step lane in the display)");
            lfo[i].sync.setTooltip("Sync: one LFO cycle per division, phase-locked to the host bar");
            lfo[i].div.setTooltip("LFO division when SYNC is on");
            lfo[i].rate.slider.setTooltip("LFO rate in Hz when SYNC is off (0.02-20 Hz)");
        }
        attack.slider.setTooltip("Attack: time to reach full after the input opens the gate");
        decay.slider.setTooltip("Decay: time to fall from full to the Sustain level while the input keeps playing");
        sustain.slider.setTooltip("Sustain: the level held while the input keeps playing (100 % = classic follower feel)");
        release.slider.setTooltip("Release: time to fall to zero after the input stops. The gate self-adjusts to the input level - the dot shows when it's open.");
        for (auto& S : slot)
        {
            S.src.setTooltip("Modulation source for this slot");
            S.dst.setTooltip("Which knob this slot moves");
            S.amt.slider.setTooltip("Amount: how far the source moves the target. Negative = inverted. LFOs swing both ways around the knob; ENV pushes one way.");
        }
        setInterceptsMouseClicks(true, true);
    }

    // Step lane editing: drag inside an LFO scope whose shape is "Steps"
    int scopeAt(juce::Point<float> p) const { for (int i = 0; i < 2; ++i) if (lfo[i].scope.contains(p) && lfo[i].shape.getSelectedId() == 8) return i; return -1; }
    void mouseDown(const juce::MouseEvent& e) override { editing = scopeAt(e.position); if (editing >= 0) { for (auto* p : lfo[editing].stepParam) p->beginChangeGesture(); mouseDrag(e); } }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (editing < 0) return;
        const auto r = lfo[editing].scope.reduced(6.0f * sc, 6.0f * sc);
        const int st = juce::jlimit(0, ModIDs::kSteps - 1, int((e.position.x - r.getX()) / r.getWidth() * ModIDs::kSteps));
        float v = juce::jlimit(-1.0f, 1.0f, 1.0f - 2.0f * (e.position.y - r.getY()) / r.getHeight());
        if (e.mods.isShiftDown()) v = std::round(v * 4.0f) / 4.0f;             // shift = quantise to quarters
        if (e.mods.isAltDown()) v = 0.0f;                                        // alt = clear
        auto* sp = lfo[editing].stepParam[st];
        sp->setValueNotifyingHost(sp->convertTo0to1(v));
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { if (editing >= 0) for (auto* p : lfo[editing].stepParam) p->endChangeGesture(); editing = -1; }

    // scale = editor width / 880
    void layoutScaled(float s)
    {
        sc = s;
        auto S = [s](float v) { return juce::roundToInt(v * s); };
        for (int i = 0; i < 2; ++i) lfo[i].glide.layout(S(18 + i * 212 + 132), S(76), S(66), S(66), S(11));   // where RATE sits; shown in Steps mode
        for (int i = 0; i < 2; ++i)
        {
            auto& L = lfo[i]; const int x0 = S(18 + i * 212);
            L.shape.setBounds(x0, S(36), S(86), S(20));
            L.sync.setBounds(x0 + S(90), S(36), S(46), S(20));
            L.div.setBounds(x0 + S(140), S(36), S(58), S(20));
            L.rate.layout(x0 + S(132), S(76), S(66), S(66), S(11));
            L.scope = juce::Rectangle<float>(float(x0), S(62.0f), S(120.0f), S(86.0f));
        }
        const int ex = S(438);
        Knob* envK[4] = { &attack, &decay, &sustain, &release };
        for (int k = 0; k < 4; ++k) envK[k]->layout(ex + S(k * 47), S(82), S(46), S(60), S(11));
        envMeter = juce::Rectangle<float>(float(ex), S(38.0f), S(142.0f), S(16.0f));
        envSrc.setBounds(ex + S(148), S(35), S(40), S(22));
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
        bool anySteps = false;
        for (int i = 0; i < 2; ++i)
        {
            const bool sync = lfo[i].sync.getToggleState();
            const bool steps = lfo[i].shape.getSelectedId() == 8;
            anySteps |= steps;
            lfo[i].div.setEnabled(sync); lfo[i].div.setAlpha(sync ? 1.0f : 0.4f);
            lfo[i].rate.slider.setAlpha(sync ? 0.35f : 1.0f);
            lfo[i].rate.slider.setVisible(! steps); lfo[i].rate.label.setVisible(! steps);
            lfo[i].glide.slider.setVisible(steps); lfo[i].glide.label.setVisible(steps);
        }
        juce::ignoreUnused(anySteps);
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
        g.setColour(lnf.accentHi); g.drawText("ENVELOPE  (ADSR)", envMeter.withY(18.0f * s).withHeight(14.0f * s), juce::Justification::centredLeft);
        if (mod.usingSidechain()) { g.setColour(mod.scPresent.load() ? lnf.accent2 : Palette::magenta); g.setFont(juce::Font(juce::FontOptions(7.5f * s, juce::Font::bold))); g.drawText(mod.scPresent.load() ? "SC CONNECTED" : "NO SC INPUT", juce::Rectangle<float>(envMeter.getX() + 92.0f * s, 18.0f * s, 100.0f * s, 14.0f * s), juce::Justification::centredLeft); }
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
        if (shape == 7)
        {
            // 16-step lane: bars from the centre line, lit step follows the phase
            const auto in = r.reduced(6.0f * s, 6.0f * s);
            const float w = in.getWidth() / ModIDs::kSteps;
            const int cur = mod.lfoStep(i);
            for (int st = 0; st < ModIDs::kSteps; ++st)
            {
                const float v = lfo[i].stepParam[st]->convertFrom0to1(lfo[i].stepParam[st]->getValue());
                const float x = in.getX() + st * w;
                const float y0 = in.getCentreY(), y1 = in.getCentreY() - v * in.getHeight() * 0.5f;
                auto bar = juce::Rectangle<float>(x + 1.0f, juce::jmin(y0, y1), w - 2.0f, std::abs(y1 - y0));
                g.setColour((st == cur ? juce::Colours::white : lnf.accent2).withAlpha(st == cur ? 0.95f : 0.55f));
                g.fillRoundedRectangle(bar, 1.5f);
                if (st % 4 == 0) { g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawVerticalLine(int(x), in.getY(), in.getBottom()); }
            }
            g.setColour(juce::Colours::white.withAlpha(0.2f)); g.drawVerticalLine(int(in.getX() + in.getWidth() * ph), in.getY(), in.getBottom());
            g.setColour(lnf.accent.withAlpha(0.5f)); g.drawRoundedRectangle(r, 8.0f * s, 1.0f);
            g.setFont(juce::Font(juce::FontOptions(7.0f * s, juce::Font::bold))); g.setColour(Palette::dim);
            g.drawText("DRAW  |  SHIFT: SNAP  |  ALT: CLEAR", r.withTrimmedTop(r.getHeight() - 10.0f * s), juce::Justification::centred);
            return;
        }
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
                case 7: v = 0.0f; break;
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
        // gate indicator
        g.setColour(mod.gateVis.load() ? lnf.accent2 : Palette::dim.withAlpha(0.4f)); g.fillEllipse(r.getRight() + 4.0f * s, r.getCentreY() - 3.0f * s, 6.0f * s, 6.0f * s);
        g.setColour(lnf.accent.withAlpha(0.5f)); g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
    }

    struct LfoUI { juce::ComboBox shape, div; juce::ToggleButton sync { "SYNC" }; Knob rate, glide; juce::Rectangle<float> scope; juce::RangedAudioParameter* stepParam[ModIDs::kSteps] {};
                   std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach, divAttach;
                   std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach; };
    struct SlotUI { juce::ComboBox src, dst; Knob amt; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAttach, dstAttach; };
    LfoUI lfo[2]; SlotUI slot[ModIDs::kSlots];
    Knob attack, decay, sustain, release;
    juce::ToggleButton envSrc { "SC" }; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> envSrcAttach;
    juce::Rectangle<float> envMeter;
    ModMatrix& mod; LookAndFeel& lnf; juce::AudioProcessorValueTreeState& state;
    int editing = -1; float sc = 1.0f;
};

} // namespace zwui
