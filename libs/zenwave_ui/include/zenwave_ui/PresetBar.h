#pragma once
// Header preset strip: < [preset browser] > SAVE  A|B  WARP
// Factory presets come from the plugin's Presets table; user presets from PresetBank.

#include "ZenWaveUI.h"
#include "PresetBank.h"
#include <functional>

namespace zwui {

class PresetBar : public juce::Component
{
public:
    struct Factory
    {
        std::function<int()> count;
        std::function<juce::String(int)> name, category;
        std::function<int()> current;
        std::function<void(int)> select;
    };

    PresetBar(PresetBank& b, Factory f, LookAndFeel& l) : bank(b), factory(std::move(f)), lnf(l)
    {
        for (auto* c : std::initializer_list<juce::Component*>{ &prev, &next, &box, &save, &ab, &warp }) addAndMakeVisible(c);
        prev.onClick = [this] { step(-1); };
        next.onClick = [this] { step(+1); };
        box.onChange = [this] { onBoxChange(); };
        save.onClick = [this] { saveDialog(); };
        ab.onClick   = [this] { bank.toggleAB(); refreshAB(); if (onStateChanged) onStateChanged(); };
        warp.onClick = [this] { const auto m = juce::ModifierKeys::getCurrentModifiers(); bank.warp(m.isCommandDown() || m.isCtrlDown() ? 1.0f : (m.isShiftDown() ? 0.55f : 0.25f)); bank.clearCurrentUser(); if (onStateChanged) onStateChanged(); };
        save.setTooltip("Save as user preset (stored in Documents/ZenWave Labs)");
        ab.setTooltip("Compare A/B. Right-click: copy current to the other slot");
        warp.setTooltip("Warp: gentle randomise. Shift = bolder, Cmd/Ctrl = anywhere. Right-click a knob to lock it.");
        ab.addMouseListener(this, false);
        rebuild();
    }

    std::function<void()> onStateChanged;

    void rebuild()
    {
        box.clear(juce::dontSendNotification);
        juce::String lastCat;
        for (int i = 0; i < factory.count(); ++i)
        {
            const auto cat = factory.category(i);
            if (cat != lastCat) { if (i > 0) box.addSeparator(); box.addSectionHeading(cat); lastCat = cat; }
            box.addItem(factory.name(i), i + 1);
        }
        userNames = bank.scan();
        if (! userNames.isEmpty())
        {
            box.addSeparator(); box.addSectionHeading("User");
            for (int i = 0; i < userNames.size(); ++i) box.addItem(userNames[i], kUserBase + i);
        }
        sync();
    }

    // Reflect processor state (called from the editor timer)
    void sync()
    {
        const auto u = bank.currentUserPreset();
        const int want = u.isNotEmpty() && userNames.contains(u) ? kUserBase + userNames.indexOf(u) : factory.current() + 1;
        if (box.getSelectedId() != want) box.setSelectedId(want, juce::dontSendNotification);
        refreshAB();
    }

    void layoutScaled(float s, int x0)
    {
        auto S = [s](int v) { return juce::roundToInt(v * s); };
        prev.setBounds(S(x0), S(22), S(24), S(30));
        box.setBounds(S(x0 + 28), S(22), S(178), S(30));
        next.setBounds(S(x0 + 210), S(22), S(24), S(30));
        save.setBounds(S(x0 + 242), S(22), S(46), S(30));
        ab.setBounds(S(x0 + 292), S(22), S(46), S(30));
        warp.setBounds(S(x0 + 342), S(22), S(52), S(30));
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.eventComponent == &ab && e.mods.isPopupMenu()) { bank.copyToOther(); }
    }

private:
    static constexpr int kUserBase = 1000;

    void onBoxChange()
    {
        const int id = box.getSelectedId();
        if (id >= kUserBase) { bank.load(userNames[id - kUserBase]); }
        else if (id > 0) { bank.clearCurrentUser(); factory.select(id - 1); }
        if (onStateChanged) onStateChanged();
    }

    void step(int dir)
    {
        // walk the item list (skipping headings/separators)
        const int n = box.getNumItems(); if (n == 0) return;
        int idx = -1; for (int i = 0; i < n; ++i) if (box.getItemId(i) == box.getSelectedId()) { idx = i; break; }
        idx = (idx + dir + n) % n;
        box.setSelectedId(box.getItemId(idx), juce::sendNotificationSync);
    }

    void saveDialog()
    {
        chooser = std::make_unique<juce::FileChooser>("Save preset", bank.dir().getChildFile(suggestName() + PresetBank::kExt), juce::String("*") + PresetBank::kExt);
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [this](const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                bank.save(f.getFileNameWithoutExtension());
                rebuild();
                if (onStateChanged) onStateChanged();
            });
    }

    juce::String suggestName() const
    {
        const auto u = bank.currentUserPreset();
        if (u.isNotEmpty()) return u;
        return factory.name(factory.current()) + " (mine)";
    }

    void refreshAB()
    {
        ab.setButtonText(bank.slot() == 0 ? "A | b" : "a | B");
        ab.setToggleState(bank.slot() == 1, juce::dontSendNotification);
    }

    PresetBank& bank; Factory factory; LookAndFeel& lnf;
    juce::TextButton prev { juce::String::charToString(0x2039) }, next { juce::String::charToString(0x203A) }, save { "SAVE" }, warp { "WARP" };
    juce::ToggleButton ab { "A | b" };
    juce::ComboBox box;
    juce::StringArray userNames;
    std::unique_ptr<juce::FileChooser> chooser;
};

} // namespace zwui
