#pragma once
// Built-in manual: a "?" button opens a scrollable overlay with a section per card and an entry
// per control. The same text feeds hover tooltips, so every knob explains itself two ways.

#include "ZenWaveUI.h"
#include <map>

namespace zwui {

struct ManualEntry { juce::String name, text; juce::String paramId; };
struct ManualSection { juce::String title, intro; std::vector<ManualEntry> entries; };

// Shared sections every ZenWave plugin has
inline ManualSection fieldSection()
{
    return { "FIELD", "What the plugin is doing to your sound, live.",
        { { "Galaxy scope", "Stereo image of the output. Up = mid (mono) energy, sideways = side (stereo) energy. A tall beam is mono, a round cloud is wide. The ring on top is phase correlation: cyan/violet = in phase and mono-safe, magenta = out of phase (check it in mono)." },
          { "Spectrum", "Real-time analyser of the output (coloured) with the dry input as a faint ghost behind it, so you can see exactly what the plugin adds. The thin white line is peak hold. Tilted +3 dB/oct so a balanced mix reads flat." },
          { "EQ curve", "The white curve is the wet-path EQ response. The two dots are the Low Cut and High Cut points - drag the knobs and watch them move." },
          { "LOW CUT", "High-pass on the wet signal. Clears mud and sub rumble from tails so the low end stays tight. 20 Hz = off.", "lowcut" },
          { "HIGH CUT", "Low-pass on the wet signal. Darkens tails and pushes them behind the dry sound. 20 kHz = off.", "highcut" },
          { "Slope (6/12/24/48 dB)", "Steepness of each cut. 6-12 dB is gentle and musical; 24-48 dB is surgical - good for keeping reverb completely out of the sub." },
          { "TILT", "Tilts the wet tone around 1 kHz: left = darker, right = brighter, +/-6 dB.", "tilt" } } };
}

inline ManualSection motionSection()
{
    return { "MOTION", "Two LFOs and an envelope follower you can route to almost any knob. Modulated knobs show a range arc and a moving cyan dot.",
        { { "LFO 1 / LFO 2", "Shape: Sine, Triangle, Saw Up, Saw Down, Square, Random (stepped) and Drift (smooth random). SYNC locks the rate to the host tempo and phase - one cycle per division, lined up to the bar. Unsynced, RATE sets it in Hz (0.02-20)." },
          { "Steps (LFO shape)", "A 16-step lane you draw straight into the LFO display: drag to set each step (bipolar), Shift = quantise to quarters, Alt = clear. One pass = the LFO's division (1/1 = a step per 1/16). GLIDE slides steps into each other. Each LFO has its own lane, so LFO 1 can walk a vowel while LFO 2 chops the mix." },
          { "ENVELOPE (ADSR)", "An envelope triggered by the input. The gate self-adjusts to your level - it opens on anything close to the recent loudest moment, so hits trigger and tails don't (the dot lights when it's open); then ATK rises to full, DEC falls to the SUS level while the sound keeps playing, and REL fades out when it stops. SUS 100 % with a short DEC feels like a classic follower; SUS 0 % turns every hit into a pluck-shaped sweep. The bar shows the live envelope." },
          { "ROUTING", "Four slots: source -> target -> amount. The amount knob is bipolar: turn left for inverted modulation. LFOs swing +/- around the knob's position; ENV pushes it one way. Several slots can hit the same target - they add." },
          { "Tip", "Synced LFO on TIME (Drift) gives octave-locked pitch wobbles; Drift LFO on WIDTH or COLOR gives slow, living tails nobody can quite place." } } };
}

inline ManualSection presetSection()
{
    return { "PRESETS", "The strip at the top of the window.",
        { { "<  >", "Step through factory and user presets." },
          { "SAVE", "Saves the current sound as a user preset in Documents / ZenWave Labs / <plugin> / Presets. It appears under 'User' in the browser." },
          { "A | B", "Two scratch slots to compare settings. Click to flip between them. Right-click to copy the current slot into the other one." },
          { "WARP", "Randomises the unlocked knobs. Click = gentle variation around what you have. Shift-click = bolder. Cmd/Ctrl-click = anywhere in range. Safe ranges keep feedback and filters out of trouble; Mix, Output and Power are never touched." },
          { "Lock", "Right-click any knob -> Lock for Warp. Locked knobs show a small padlock and keep their value. Right-click also offers Reset to default." },
          { "Power", "The round button at the top right. Off = clean bypass with a 12 ms crossfade, so you can A/B without clicks." } } };
}

// Overlay component ------------------------------------------------------------------------
class ManualOverlay : public juce::Component
{
public:
    ManualOverlay(LookAndFeel& l) : lnf(l)
    {
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&content, false);
        viewport.setScrollBarsShown(true, false);
        addAndMakeVisible(close);
        close.onClick = [this] { setVisible(false); };
        setVisible(false);
        setWantsKeyboardFocus(true);
    }

    void set(const juce::String& title, const juce::String& tagline, std::vector<ManualSection> s)
    {
        content.title = title; content.tagline = tagline; content.sections = std::move(s); content.lnf = &lnf;
        content.setSize(juce::jmax(100, getWidth() - 120), 10); content.relayout();
    }

    void show() { setVisible(true); toFront(true); grabKeyboardFocus(); }
    bool keyPressed(const juce::KeyPress& k) override { if (k == juce::KeyPress::escapeKey) { setVisible(false); return true; } return false; }
    void mouseDown(const juce::MouseEvent& e) override { if (! panel().contains(e.position)) setVisible(false); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.72f));
        auto r = panel();
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillRoundedRectangle(r.translated(0, 6), 18.0f);
        g.setColour(Palette::bg2.withAlpha(0.97f)); g.fillRoundedRectangle(r, 18.0f);
        g.setGradientFill(juce::ColourGradient(lnf.accent.withAlpha(0.9f), r.getX(), r.getY(), lnf.accent2.withAlpha(0.5f), r.getRight(), r.getBottom(), false));
        g.drawRoundedRectangle(r, 18.0f, 1.2f);
        g.setColour(Palette::dim);
        g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
        g.drawText("CLICK OUTSIDE OR PRESS ESC TO CLOSE", r.removeFromBottom(22.0f), juce::Justification::centred);
    }

    void resized() override
    {
        auto r = panel().toNearestInt();
        close.setBounds(r.getRight() - 40, r.getY() + 12, 28, 28);
        viewport.setBounds(r.reduced(28, 24).withTrimmedBottom(10));
        content.setSize(viewport.getWidth() - 14, 10); content.relayout();
    }

private:
    juce::Rectangle<float> panel() const { return getLocalBounds().toFloat().reduced(getWidth() * 0.07f, getHeight() * 0.06f); }

    struct Content : juce::Component
    {
        juce::String title, tagline; std::vector<ManualSection> sections; LookAndFeel* lnf = nullptr;
        struct Block { juce::TextLayout layout; float y; juce::Colour col; bool heading; bool rule; };
        std::vector<Block> blocks;

        void relayout()
        {
            if (! lnf) return;
            blocks.clear();
            const float w = float(getWidth()) - 8.0f;
            float y = 4.0f;
            auto add = [&](const juce::String& txt, juce::Font f, juce::Colour c, bool heading = false, float gapAfter = 6.0f, bool rule = false)
            {
                juce::AttributedString as; as.setJustification(juce::Justification::topLeft); as.append(txt, f, c);
                Block b; b.layout.createLayout(as, w); b.y = y; b.col = c; b.heading = heading; b.rule = rule;
                y += b.layout.getHeight() + gapAfter; blocks.push_back(std::move(b));
            };
            juce::Font big(juce::FontOptions(26.0f, juce::Font::bold)); big.setExtraKerningFactor(0.08f);
            add(title + "  MANUAL", big, Palette::text, true, 2.0f);
            add(tagline, juce::Font(juce::FontOptions(12.5f)), lnf->accentHi, false, 18.0f);
            for (auto& s : sections)
            {
                add(s.title, juce::Font(juce::FontOptions(13.0f, juce::Font::bold)), lnf->accent2, true, 3.0f, true);
                if (s.intro.isNotEmpty()) add(s.intro, juce::Font(juce::FontOptions(12.0f, juce::Font::italic)), Palette::dim, false, 8.0f);
                for (auto& e : s.entries)
                {
                    juce::AttributedString as; as.setJustification(juce::Justification::topLeft);
                    as.append(e.name + "   ", juce::Font(juce::FontOptions(12.0f, juce::Font::bold)), lnf->accentHi);
                    as.append(e.text, juce::Font(juce::FontOptions(12.0f)), Palette::text.withAlpha(0.9f));
                    Block b; b.layout.createLayout(as, w); b.y = y; b.col = Palette::text; b.heading = false; b.rule = false;
                    y += b.layout.getHeight() + 7.0f; blocks.push_back(std::move(b));
                }
                y += 12.0f;
            }
            setSize(getWidth(), int(y) + 10);
            repaint();
        }
        void paint(juce::Graphics& g) override
        {
            for (auto& b : blocks)
            {
                if (b.rule) { g.setColour(lnf->accent.withAlpha(0.35f)); g.drawHorizontalLine(int(b.y) - 6, 0.0f, float(getWidth())); }
                b.layout.draw(g, juce::Rectangle<float>(4.0f, b.y, float(getWidth()) - 8.0f, b.layout.getHeight()));
            }
        }
    } content;

    juce::Viewport viewport;
    juce::TextButton close { "x" };
    LookAndFeel& lnf;
};

// Apply tooltips from the manual to a set of knobs by parameter id
inline void applyTooltips(const std::vector<ManualSection>& sections, const std::vector<Knob*>& knobs)
{
    std::map<juce::String, juce::String> byId;
    for (auto& s : sections) for (auto& e : s.entries) if (e.paramId.isNotEmpty()) byId[e.paramId] = e.name + " - " + e.text;
    for (auto* k : knobs) if (auto it = byId.find(k->paramId); it != byId.end()) k->slider.setTooltip(it->second);
}

} // namespace zwui
