#pragma once
// The NEBULA centrepiece: oscillator A's wavetable as a planet of stacked rings, oscillator B as its
// orbital ring, the carved harmonic spectrum as an outer halo (with the 8 hand-placed carve bands as
// draggable arcs), and playing voices as moons.

#include <JuceHeader.h>
#include <zenwave_ui/ZenWaveUI.h>
#include "PluginProcessor.h"

class PlanetView : public juce::Component, public juce::SettableTooltipClient
{
public:
    PlanetView(NebulaAudioProcessor& p, zwui::LookAndFeel& l) : proc(p), lnf(l)
    {
        for (int i = 0; i < nebula::kCarveBands; ++i)
        {
            cOn[i] = p.apvts.getParameter(ParamIDs::carveBandOn(i)); cC[i] = p.apvts.getParameter(ParamIDs::carveBandCentre(i));
            cW[i] = p.apvts.getParameter(ParamIDs::carveBandWidth(i)); cG[i] = p.apvts.getParameter(ParamIDs::carveBandGain(i));
        }
        posA = p.apvts.getParameter("aposition"); posB = p.apvts.getParameter("bposition");
        tableA = p.apvts.getParameter("atable"); tableB = p.apvts.getParameter("btable");
        setTooltip("The planet is oscillator A's wavetable (lit ring = POSITION), the orbit is oscillator B. Outer halo = the harmonic spectrum. CARVE: click the halo to place a band, drag to move it, drag up/down for gain, Shift-drag for width, double-click to remove.");
    }
    void tick(float dt) { t += dt; repaint(); }

    // halo geometry: harmonic 1..256 maps to angle -150 .. +150 degrees (gap at the bottom)
    static float angleFor(float harmonic) { return juce::degreesToRadians(-150.0f + 300.0f * std::log2(juce::jlimit(1.0f, 256.0f, harmonic)) / 8.0f); }
    static float harmonicFor(float angle) { const float d = juce::jlimit(-150.0f, 150.0f, juce::radiansToDegrees(angle)); return std::pow(2.0f, (d + 150.0f) / 300.0f * 8.0f); }
    float plain(juce::RangedAudioParameter* p) const { return p->convertFrom0to1(p->getValue()); }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const auto c = b.getCentre();
        const float R = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f / 1.62f;
        const float rot = t * 0.25f;
        // ambient glow
        g.setGradientFill(juce::ColourGradient(lnf.accent.withAlpha(0.18f), c.x, c.y, juce::Colours::transparentBlack, c.x + R * 1.7f, c.y, true));
        g.fillEllipse(c.x - R * 1.7f, c.y - R * 1.7f, R * 3.4f, R * 3.4f);

        // ---- planet: oscillator A rings ----
        auto tabA = proc.tableForUI(0), tabB = proc.tableForUI(1);
        const float pA = plain(posA), pB = plain(posB);
        const int litA = int(pA * (nebula::kFrames - 1) + 0.5f);
        if (tabA)
        {
            for (int pass = 1; pass < 2; ++pass)
                for (int f = 0; f < nebula::kFrames; f += 2)
                {
                    const float lat = (f - (nebula::kFrames - 1) * 0.5f) / ((nebula::kFrames - 1) * 0.5f);   // -1..1
                    const float ry = lat * R * 0.95f, rx = std::sqrt(std::max(0.0f, 1.0f - lat * lat)) * R;
                    const bool lit = std::abs(f - litA) <= 1;
                    const float* fr = tabA->frame(0, f);
                    juce::Path ring;
                    const int N = 96;
                    for (int k = 0; k <= N; ++k)
                    {
                        const float a = juce::MathConstants<float>::twoPi * k / N;
                        // back pass draws the half facing away (sin(a) < 0), front pass the near half
                        const float ph = std::fmod(a / juce::MathConstants<float>::twoPi + rot, 1.0f);
                        const float w = fr[int(ph * nebula::kFrameLen) & (nebula::kFrameLen - 1)];
                        const float rad = 1.0f + w * 0.16f;
                        const float x = c.x + std::cos(a) * rx * rad;
                        const float y = c.y + ry + std::sin(a) * rx * 0.28f * rad;
                        if (k == 0) ring.startNewSubPath(x, y); else ring.lineTo(x, y);
                    }
                    const float depth = 1.0f; juce::ignoreUnused(pass);
                    const auto col = lit ? juce::Colours::white.interpolatedWith(lnf.accent2, 0.3f) : zwui::aurora(0.5f + 0.5f * lat, lnf.accent, lnf.accent2);
                    if (lit) zwui::glowStroke(g, ring, col.withAlpha(0.95f * depth), 1.8f, 1.2f);
                    else { g.setColour(col.withAlpha((0.22f + 0.25f * (1.0f - std::abs(lat))) * depth)); g.strokePath(ring, juce::PathStrokeType(1.0f)); }
                }
        }
        // ---- oscillator B: orbital ring (tilted), its waveform as the ring's thickness ----
        if (tabB)
        {
            const int fB = int(pB * (nebula::kFrames - 1) + 0.5f);
            const float* fr = tabB->frame(0, fB);
            juce::Path orbit; const int N = 160;
            for (int k = 0; k <= N; ++k)
            {
                const float a = juce::MathConstants<float>::twoPi * k / N;
                const float ph = std::fmod(a / juce::MathConstants<float>::twoPi - rot * 0.6f + 1.0f, 1.0f);
                const float w = fr[int(ph * nebula::kFrameLen) & (nebula::kFrameLen - 1)];
                const float rr = R * 1.28f * (1.0f + w * 0.07f);
                const float x = c.x + std::cos(a) * rr, y = c.y + std::sin(a) * rr * 0.34f + R * 0.05f;
                if (k == 0) orbit.startNewSubPath(x, y); else orbit.lineTo(x, y);
            }
            zwui::glowStroke(g, orbit, lnf.accent2.withAlpha(0.75f), 1.4f, 0.8f);
        }
        // ---- moons: playing voices ----
        for (int i = 0; i < NebulaAudioProcessor::kVoices; ++i)
        {
            const float pitch = proc.voicePitch[size_t(i)].load(); if (pitch < 0) continue;
            const float lvl = proc.voiceLevel[size_t(i)].load();
            const float a = juce::MathConstants<float>::twoPi * std::fmod(pitch / 12.0f, 1.0f) + t * 0.5f;
            const float rr = R * (1.08f + 0.3f * (pitch - 36.0f) / 60.0f);
            const auto p = juce::Point<float>(c.x + std::cos(a) * rr, c.y + std::sin(a) * rr * 0.5f);
            const float sz = 3.0f + 7.0f * lvl;
            g.setColour(lnf.accent2.withAlpha(0.25f * lvl)); g.fillEllipse(p.x - sz * 1.8f, p.y - sz * 1.8f, sz * 3.6f, sz * 3.6f);
            g.setColour(juce::Colours::white.withAlpha(0.4f + 0.6f * lvl)); g.fillEllipse(p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz);
        }
        // ---- halo: harmonic spectrum of osc A at the current position, after carve ----
        {
            const float Rh = R * 1.46f;
            juce::Random rng(1234 + int(plain(tableA)) * 99 + litA);
            std::vector<float> amp(nebula::kPartials), ph(nebula::kPartials);
            nebula::recipe(int(plain(tableA)), pA, amp.data(), ph.data(), rng);
            const auto bands = proc.carveBands(); const bool carveOn = proc.apvts.getRawParameterValue(ParamIDs::carveOn)->load() > 0.5f;
            float pk = 1e-4f; for (int h = 0; h < 256; ++h) pk = std::max(pk, amp[size_t(h)]);
            for (int h = 1; h <= 256; ++h)
            {
                const float raw = amp[size_t(h - 1)] / pk;
                const float gn = carveOn ? nebula::carveGainAt(bands, h) : 1.0f;
                const float v = std::pow(raw * gn, 0.5f);
                if (v < 0.01f && raw < 0.01f) continue;
                const float a = angleFor(float(h)) - juce::MathConstants<float>::halfPi;
                const auto p0 = c.getPointOnCircumference(Rh, a + juce::MathConstants<float>::halfPi), p1 = c.getPointOnCircumference(Rh + 4.0f + v * R * 0.26f, a + juce::MathConstants<float>::halfPi);
                g.setColour(zwui::aurora(v, lnf.accent, lnf.accent2).withAlpha(0.35f + 0.6f * v));
                g.drawLine({ p0, p1 }, h < 16 ? 2.0f : 1.2f);
            }
            // halo baseline
            juce::Path base; base.addCentredArc(c.x, c.y, Rh, Rh, 0.0f, juce::degreesToRadians(-150.0f), juce::degreesToRadians(150.0f), true);
            g.setColour(lnf.accent.withAlpha(0.35f)); g.strokePath(base, juce::PathStrokeType(1.0f));
            g.setFont(juce::Font(juce::FontOptions(7.5f, juce::Font::bold))); g.setColour(zwui::Palette::dim);
            for (int h : { 1, 2, 4, 8, 16, 32, 64, 128 }) { const auto p = c.getPointOnCircumference(Rh - 9.0f, angleFor(float(h))); g.drawText("H" + juce::String(h), int(p.x) - 10, int(p.y) - 5, 20, 10, juce::Justification::centred); }
            // carve bands as arcs: cut dips inward, boost bulges outward
            for (int i = 0; i < nebula::kCarveBands; ++i)
            {
                if (cOn[i]->getValue() < 0.5f) continue;
                const float centre = plain(cC[i]), w = plain(cW[i]), gain = plain(cG[i]);
                const float a0 = angleFor(centre / std::pow(2.0f, w * 0.5f)), a1 = angleFor(centre * std::pow(2.0f, w * 0.5f));
                const float gdb = juce::Decibels::gainToDecibels(gain, -30.0f);
                const float off = juce::jlimit(-1.0f, 1.0f, gdb / 18.0f) * R * 0.22f;
                juce::Path arc; arc.addCentredArc(c.x, c.y, Rh + off, Rh + off, 0.0f, a0, a1, true);
                const bool hot = drag == i || hover == i;
                const auto col = gain < 1.0f ? zwui::Palette::magenta : lnf.accent2;
                zwui::glowStroke(g, arc, (hot ? juce::Colours::white : col).withAlpha(0.9f), hot ? 3.0f : 2.2f, 0.9f);
                const auto hp = c.getPointOnCircumference(Rh + off, angleFor(centre));
                g.setColour(hot ? juce::Colours::white : col); g.fillEllipse(hp.x - 5.0f, hp.y - 5.0f, 10.0f, 10.0f);
                g.setColour(zwui::Palette::bg); g.setFont(juce::Font(juce::FontOptions(7.5f, juce::Font::bold))); g.drawText(juce::String(i + 1), int(hp.x) - 5, int(hp.y) - 5, 10, 10, juce::Justification::centred);
                if (hot) { g.setColour(juce::Colours::white); g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold))); g.drawText("H" + juce::String(centre, 1) + "  " + juce::String(w, 2) + " oct  " + (gain < 0.002f ? juce::String("cut") : juce::String(gdb, 1) + " dB"), int(hp.x) + 10, int(hp.y) - 7, 150, 14, juce::Justification::left); }
            }
        }
        // title
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold))); g.setColour(zwui::Palette::dim);
        g.drawText(nebula::tableNames()[int(plain(tableA))].toUpperCase() + "   /   " + nebula::tableNames()[int(plain(tableB))].toUpperCase(), b.withTrimmedBottom(4.0f).removeFromBottom(12.0f), juce::Justification::centred);
    }

    // ---- interaction on the halo ----
    int bandAt(juce::Point<float> p) const
    {
        const auto c = getLocalBounds().toFloat().getCentre(); const float R = juce::jmin(getWidth(), getHeight()) * 0.5f / 1.62f, Rh = R * 1.46f;
        for (int i = 0; i < nebula::kCarveBands; ++i)
        {
            if (cOn[i]->getValue() < 0.5f) continue;
            const float gdb = juce::Decibels::gainToDecibels(plain(cG[i]), -30.0f), off = juce::jlimit(-1.0f, 1.0f, gdb / 18.0f) * R * 0.22f;
            if (c.getPointOnCircumference(Rh + off, angleFor(plain(cC[i]))).getDistanceFrom(p) < 10.0f) return i;
        }
        return -1;
    }
    bool onHalo(juce::Point<float> p) const
    {
        const auto c = getLocalBounds().toFloat().getCentre(); const float R = juce::jmin(getWidth(), getHeight()) * 0.5f / 1.62f, Rh = R * 1.46f;
        const float d = c.getDistanceFrom(p); const float ang = juce::radiansToDegrees(c.getAngleToPoint(p));
        return d > Rh - R * 0.3f && d < Rh + R * 0.35f && ang > -150.0f && ang < 150.0f;
    }
    void mouseMove(const juce::MouseEvent& e) override { hover = bandAt(e.position); setMouseCursor(hover >= 0 ? juce::MouseCursor::DraggingHandCursor : onHalo(e.position) ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor); repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown(const juce::MouseEvent& e) override
    {
        drag = bandAt(e.position);
        if (drag < 0 && onHalo(e.position))
        {
            for (int i = 0; i < nebula::kCarveBands; ++i) if (cOn[i]->getValue() < 0.5f)   // first free band
            {
                const auto c = getLocalBounds().toFloat().getCentre();
                setPlain(cC[i], harmonicFor(c.getAngleToPoint(e.position))); setPlain(cW[i], 0.4f); setPlain(cG[i], 0.25f);
                cOn[i]->beginChangeGesture(); cOn[i]->setValueNotifyingHost(1.0f); cOn[i]->endChangeGesture();
                drag = i; break;
            }
        }
        if (drag >= 0) { for (auto* p : { cC[drag], cW[drag], cG[drag] }) p->beginChangeGesture(); startW = plain(cW[drag]); startG = plain(cG[drag]); startY = e.position.y; }
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (drag < 0) return;
        const auto c = getLocalBounds().toFloat().getCentre();
        if (e.mods.isShiftDown()) setPlain(cW[drag], juce::jlimit(0.05f, 2.0f, startW * std::pow(2.0f, -(e.position.y - startY) / 40.0f)));
        else
        {
            setPlain(cC[drag], harmonicFor(c.getAngleToPoint(e.position)));
            // vertical drag = gain: up = boost, down = cut (relative to where the drag started)
            const float gdb = juce::jlimit(-30.0f, 12.0f, juce::Decibels::gainToDecibels(startG, -30.0f) - (e.position.y - startY) * 0.25f);
            setPlain(cG[drag], gdb <= -29.5f ? 0.0f : juce::Decibels::decibelsToGain(gdb));
        }
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { if (drag >= 0) for (auto* p : { cC[drag], cW[drag], cG[drag] }) p->endChangeGesture(); drag = -1; }
    void mouseDoubleClick(const juce::MouseEvent& e) override { const int i = bandAt(e.position); if (i >= 0) { cOn[i]->beginChangeGesture(); cOn[i]->setValueNotifyingHost(0.0f); cOn[i]->endChangeGesture(); } }

private:
    void setPlain(juce::RangedAudioParameter* p, float v) { p->setValueNotifyingHost(p->convertTo0to1(v)); }
    NebulaAudioProcessor& proc; zwui::LookAndFeel& lnf; float t = 0.0f;
    juce::RangedAudioParameter* cOn[nebula::kCarveBands]{}, * cC[nebula::kCarveBands]{}, * cW[nebula::kCarveBands]{}, * cG[nebula::kCarveBands]{};
    juce::RangedAudioParameter* posA{}, * posB{}, * tableA{}, * tableB{};
    int drag = -1, hover = -1; float startW = 0.4f, startG = 1.0f, startY = 0.0f;
};
