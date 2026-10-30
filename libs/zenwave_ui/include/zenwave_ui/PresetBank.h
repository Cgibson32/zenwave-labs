#pragma once
// ZenWave preset bank: user presets on disk, A/B compare, Warp randomiser with per-parameter
// locks. Lives in the processor (owns a reference to the APVTS); the editor drives it.
//
//   ~/Documents/ZenWave Labs/<Plugin>/Presets/<name>.zwpreset   (XML of the APVTS state)

#include <juce_audio_processors/juce_audio_processors.h>
#include <set>
#include <map>

namespace zwui {

class PresetBank
{
public:
    PresetBank(juce::AudioProcessorValueTreeState& s, juce::String pluginName) : apvts(s), plugin(std::move(pluginName)) {}

    static constexpr const char* kExt = ".zwpreset";

    juce::File dir() const
    {
        auto d = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("ZenWave Labs").getChildFile(plugin).getChildFile("Presets");
        d.createDirectory();
        return d;
    }

    // ---- user presets ----
    juce::StringArray scan() const
    {
        juce::StringArray names;
        for (auto& f : dir().findChildFiles(juce::File::findFiles, false, juce::String("*") + kExt)) names.add(f.getFileNameWithoutExtension());
        names.sortNatural();
        return names;
    }

    bool save(const juce::String& name)
    {
        auto state = apvts.copyState();
        state.setProperty("zwPlugin", plugin, nullptr);
        state.setProperty("zwName", name, nullptr);
        state.setProperty("locks", locksToString(), nullptr);
        if (auto xml = state.createXml())
        {
            const bool ok = xml->writeTo(dir().getChildFile(juce::File::createLegalFileName(name) + kExt));
            if (ok) { currentUser = name; dirtySince = false; }
            return ok;
        }
        return false;
    }

    bool load(const juce::String& name)
    {
        auto f = dir().getChildFile(juce::File::createLegalFileName(name) + kExt);
        if (! f.existsAsFile()) return false;
        if (auto xml = juce::XmlDocument::parse(f))
            if (xml->hasTagName(apvts.state.getType()))
            {
                auto tree = juce::ValueTree::fromXml(*xml);
                applyTree(tree);
                currentUser = name; dirtySince = false;
                return true;
            }
        return false;
    }

    bool remove(const juce::String& name)
    {
        const bool ok = dir().getChildFile(juce::File::createLegalFileName(name) + kExt).deleteFile();
        if (ok && currentUser == name) currentUser.clear();
        return ok;
    }

    juce::String currentUserPreset() const { return currentUser; }
    void clearCurrentUser() { currentUser.clear(); }

    // ---- A/B ----
    int  slot() const { return abSlot; }
    void toggleAB()
    {
        abState[abSlot] = apvts.copyState();
        abSlot ^= 1;
        if (abState[abSlot].isValid()) applyTree(abState[abSlot]);
        else abState[abSlot] = apvts.copyState();      // first visit to the other slot: starts as a copy
    }
    void copyToOther() { abState[abSlot ^ 1] = apvts.copyState(); }

    // ---- Warp (randomise) ----
    // Parameters that are never randomised regardless of locks (power, output trims, etc.)
    void setAlwaysLocked(std::initializer_list<const char*> ids) { for (auto* id : ids) alwaysLocked.insert(id); }
    // Optional safe range per parameter in plain units, so e.g. feedback never lands above self-oscillation
    void setSafeRange(const char* id, float plainMin, float plainMax) { safe[id] = { plainMin, plainMax }; }

    bool isLocked(const juce::String& id) const { return locks.count(id) > 0; }
    void setLocked(const juce::String& id, bool on) { if (on) locks.insert(id); else locks.erase(id); }
    void toggleLock(const juce::String& id) { setLocked(id, ! isLocked(id)); }

    // amount 0..1: 0.25 = gentle variation around the current sound, 1 = anywhere in range
    void warp(float amount)
    {
        amount = juce::jlimit(0.0f, 1.0f, amount);
        for (auto* p : apvts.processor.getParameters())
        {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
            if (rp == nullptr) continue;
            const auto id = rp->paramID;
            if (locks.count(id) || alwaysLocked.count(id)) continue;

            float lo = 0.0f, hi = 1.0f;
            if (auto it = safe.find(id); it != safe.end()) { lo = rp->convertTo0to1(it->second.first); hi = rp->convertTo0to1(it->second.second); }

            const bool discrete = dynamic_cast<juce::AudioParameterBool*>(rp) || dynamic_cast<juce::AudioParameterChoice*>(rp);
            float v;
            if (discrete)
            {
                // flip/choose with probability scaled by amount; tame at low amounts
                if (rng.nextFloat() > amount * 0.6f) continue;
                v = rng.nextFloat();
            }
            else
            {
                const float cur = rp->getValue();
                const float spread = amount >= 0.999f ? 1.0f : amount * 0.9f;
                // triangular noise around the current value, widening with amount
                const float noise = (rng.nextFloat() + rng.nextFloat() - 1.0f) * spread;
                v = amount >= 0.999f ? rng.nextFloat() : cur + noise;
            }
            v = juce::jlimit(lo, hi, v);
            rp->beginChangeGesture(); rp->setValueNotifyingHost(v); rp->endChangeGesture();
        }
        dirtySince = true;
    }

    // ---- state persistence of locks/AB (call from get/setStateInformation) ----
    void writeTo(juce::ValueTree& state) const { state.setProperty("locks", locksToString(), nullptr); state.setProperty("userPreset", currentUser, nullptr); }
    void readFrom(const juce::ValueTree& state)
    {
        locks.clear();
        for (auto& t : juce::StringArray::fromTokens(state.getProperty("locks").toString(), ",", "")) if (t.isNotEmpty()) locks.insert(t);
        currentUser = state.getProperty("userPreset").toString();
    }

private:
    void applyTree(const juce::ValueTree& tree)
    {
        // Set through the parameters (not replaceState) so hosts and attachments see every change
        for (auto* p : apvts.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                auto child = tree.getChildWithProperty("id", rp->paramID);
                if (! child.isValid()) continue;
                const float plain = float(child.getProperty("value"));
                rp->beginChangeGesture(); rp->setValueNotifyingHost(rp->convertTo0to1(plain)); rp->endChangeGesture();
            }
        if (tree.hasProperty("locks")) { locks.clear(); for (auto& t : juce::StringArray::fromTokens(tree.getProperty("locks").toString(), ",", "")) if (t.isNotEmpty()) locks.insert(t); }
    }
    juce::String locksToString() const { juce::StringArray a; for (auto& l : locks) a.add(l); return a.joinIntoString(","); }

    juce::AudioProcessorValueTreeState& apvts;
    juce::String plugin, currentUser;
    std::set<juce::String> locks, alwaysLocked;
    std::map<juce::String, std::pair<float, float>> safe;
    juce::ValueTree abState[2];
    int abSlot = 0;
    bool dirtySince = false;
    juce::Random rng;
};

} // namespace zwui
