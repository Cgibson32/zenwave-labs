#pragma once
// Perform pad: an XY "throw" surface. Drag = momentary override of two parameters
// (e.g. time × feedback); release = the sound glides home over a synced division.
// Drives three automatable parameters (padx, pady, padhold) so hosts can record it and
// controllers can be mapped to it. The processor owns the glide (sample-accurate); the
// pad only draws the effective position it reports back.

#include "ZenWaveUI.h"
#include "ProcessorHelpers.h"
#include <deque>

namespace zwui {

struct PadIDs
{
    static constexpr auto x = "padx", y = "pady", hold = "padhold", glide = "padglide";
    static juce::StringArray glideNames() { return { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" }; }
    static double glideBeats(int i) { static const double b[] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0 }; return b[juce::jlimit(0, 5, i)]; }
};

inline void addPadParams(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    using P = juce::AudioParameterFloat; using Rng = juce::NormalisableRange<float>;
    params.push_back(std::make_unique<P>(juce::ParameterID{ PadIDs::x, 1 }, "Pad X", Rng{ 0.0f, 1.0f, 0.0001f }, 0.5f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ PadIDs::y, 1 }, "Pad Y", Rng{ 0.0f, 1.0f, 0.0001f }, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ PadIDs::hold, 1 }, "Pad Hold", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ PadIDs::glide, 1 }, "Pad Glide", PadIDs::glideNames(), 2));
}

// Processor side: ramps the override amount in (15 ms) on hold, out over the glide division on release.
class PadEngine
{
public:
    void attach(juce::AudioProcessorValueTreeState& apvts)
    {
        px = apvts.getRawParameterValue(PadIDs::x); py = apvts.getRawParameterValue(PadIDs::y);
        ph = apvts.getRawParameterValue(PadIDs::hold); pg = apvts.getRawParameterValue(PadIDs::glide);
    }
    void prepare(double sampleRate) { sr = sampleRate; amt = 0.0f; }

    // Advance by n samples; returns the override amount 0..1 (eased)
    float process(int n, double bpm)
    {
        const bool hold = ph->load() > 0.5f;
        if (hold) { amt = std::min(1.0f, amt + float(n) / float(sr * 0.015)); gliding = false; }
        else
        {
            const double glideSec = 60.0 / juce::jlimit(20.0, 300.0, bpm) * PadIDs::glideBeats(int(pg->load()));
            if (amt > 0.0f) { amt = std::max(0.0f, amt - float(n) / float(sr * glideSec)); gliding = amt > 0.0f; }
        }
        const float e = amt * amt * (3.0f - 2.0f * amt);    // smoothstep ease
        eased.store(e);
        return e;
    }
    float x() const { return px->load(); }
    float y() const { return py->load(); }
    bool  held() const { return ph->load() > 0.5f; }
    std::atomic<float> eased { 0.0f };
    std::atomic<float> effX { 0.5f }, effY { 0.5f };   // effective position (processor writes, pad draws)
    bool gliding = false;
private:
    std::atomic<float>* px{}, * py{}, * ph{}, * pg{};
    double sr = 48000.0; float amt = 0.0f;
};

class PerformPad : public juce::Component, public juce::SettableTooltipClient
{
public:
    PerformPad(juce::AudioProcessorValueTreeState& s, PadEngine& e, LookAndFeel& l, juce::String xName, juce::String yName)
        : apvts(s), eng(e), lnf(l), xLabel(std::move(xName)), yLabel(std::move(yName))
    {
        pX = apvts.getParameter(PadIDs::x); pY = apvts.getParameter(PadIDs::y); pH = apvts.getParameter(PadIDs::hold);
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
        setTooltip("Perform pad: drag to throw (" + xLabel + " across, " + yLabel + " up). Let go and it glides home over the GLIDE division. Automatable and MIDI-mappable (Pad X / Pad Y / Pad Hold).");
    }

    // Home position (where the knobs are) so the pad can show where "home" is
    void setHome(float hx, float hy) { homeX = hx; homeY = hy; }
    // Optional snap columns (synced divisions)
    void setColumns(int n) { columns = n; }

    void mouseDown(const juce::MouseEvent& e) override
    {
        for (auto* p : { pX, pY, pH }) p->beginChangeGesture();
        pH->setValueNotifyingHost(1.0f);
        mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        const auto r = getLocalBounds().toFloat().reduced(6.0f);
        pX->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth()));
        pY->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, 1.0f - (e.position.y - r.getY()) / r.getHeight()));
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        pH->setValueNotifyingHost(0.0f);
        for (auto* p : { pX, pY, pH }) p->endChangeGesture();
    }

    void tick()
    {
        trail.push_front({ eng.effX.load(), eng.effY.load() });
        if (trail.size() > 18) trail.pop_back();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const auto in = r.reduced(6.0f);
        const float e = eng.eased.load();
        g.setColour(Palette::bg.withAlpha(0.7f)); g.fillRoundedRectangle(r, 10.0f);
        // grid
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        const int cols = columns > 0 ? columns : 4;
        for (int i = 1; i < cols; ++i) g.drawVerticalLine(int(in.getX() + in.getWidth() * i / cols), in.getY(), in.getBottom());
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine(int(in.getY() + in.getHeight() * i / 4), in.getX(), in.getRight());
        // danger zone (top = self-oscillation)
        g.setGradientFill(juce::ColourGradient(Palette::magenta.withAlpha(0.14f), in.getX(), in.getY(), juce::Colours::transparentBlack, in.getX(), in.getY() + in.getHeight() * 0.18f, false));
        g.fillRect(in.withHeight(in.getHeight() * 0.18f));
        // labels
        g.setFont(juce::Font(juce::FontOptions(7.5f, juce::Font::bold)));
        g.setColour(Palette::dim.withAlpha(0.8f));
        g.drawText(xLabel, in.withTop(in.getBottom() - 10.0f).translated(0, -1), juce::Justification::centredRight);
        {
            juce::Graphics::ScopedSaveState ss(g);
            g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, in.getX() + 6.0f, in.getCentreY()));
            g.drawText(yLabel, juce::Rectangle<float>(in.getX() + 6.0f - 40.0f, in.getCentreY() - 6.0f, 80.0f, 12.0f), juce::Justification::centred);
        }
        auto toPt = [&](float x, float y) { return juce::Point<float>(in.getX() + x * in.getWidth(), in.getY() + (1.0f - y) * in.getHeight()); };
        // home marker
        {
            const auto h = toPt(homeX, homeY);
            g.setColour(lnf.accent.withAlpha(0.7f));
            g.drawEllipse(h.x - 4.0f, h.y - 4.0f, 8.0f, 8.0f, 1.0f);
            g.drawLine(h.x - 7.0f, h.y, h.x - 4.0f, h.y, 1.0f); g.drawLine(h.x + 4.0f, h.y, h.x + 7.0f, h.y, 1.0f);
            g.drawLine(h.x, h.y - 7.0f, h.x, h.y - 4.0f, 1.0f); g.drawLine(h.x, h.y + 4.0f, h.x, h.y + 7.0f, 1.0f);
        }
        // comet trail of the effective position
        for (size_t i = 1; i < trail.size(); ++i)
        {
            const float a = (1.0f - float(i) / float(trail.size())) * (0.15f + 0.6f * e);
            const auto p0 = toPt(trail[i - 1].first, trail[i - 1].second), p1 = toPt(trail[i].first, trail[i].second);
            g.setColour(lnf.accent2.withAlpha(a)); g.drawLine({ p0, p1 }, 2.0f + 2.0f * (1.0f - float(i) / float(trail.size())));
        }
        // live dot
        if (! trail.empty())
        {
            const auto p = toPt(trail.front().first, trail.front().second);
            const auto col = aurora(e, lnf.accent, lnf.accent2);
            g.setGradientFill(juce::ColourGradient(col.withAlpha(0.35f + 0.4f * e), p.x, p.y, juce::Colours::transparentBlack, p.x + 14.0f + 10.0f * e, p.y, true));
            g.fillEllipse(p.x - 24.0f, p.y - 24.0f, 48.0f, 48.0f);
            g.setColour(juce::Colours::white); g.fillEllipse(p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f);
        }
        g.setColour((eng.held() ? lnf.accent2 : lnf.accent).withAlpha(0.6f + 0.4f * e));
        g.drawRoundedRectangle(r, 10.0f, 1.0f + e);
        if (e < 0.01f && ! eng.held())
        {
            g.setColour(Palette::dim.withAlpha(0.6f));
            g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
            g.drawText("THROW", juce::Rectangle<float>(in.getX() + 12.0f, in.getBottom() - 12.0f, 50.0f, 12.0f), juce::Justification::centredLeft);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts; PadEngine& eng; LookAndFeel& lnf;
    juce::RangedAudioParameter* pX{}, * pY{}, * pH{};
    juce::String xLabel, yLabel;
    float homeX = 0.5f, homeY = 0.5f; int columns = 0;
    std::deque<std::pair<float, float>> trail;
};

} // namespace zwui
