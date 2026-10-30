#pragma once
// ORBIT: ZenWave plugins on the same machine find each other through a tiny shared-memory
// bus (a memory-mapped file in the temp folder). Any instance with Orbit on that freezes,
// freezes every other Orbit instance at the same moment - Drift's loop and Umbra's tail
// lock together. Also exposes a heartbeat so the UI can show how many plugins are in orbit.

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cstring>

namespace zwui {

class OrbitBus
{
public:
    static constexpr auto orbitParamId = "orbit";
    static constexpr int kSlots = 24;

    struct Shared
    {
        std::atomic<uint32_t> magic;
        std::atomic<uint32_t> freezeSeq;      // bumps on every freeze change
        std::atomic<uint32_t> freezeOn;
        std::atomic<uint32_t> freezeFrom;     // instance id that published it
        struct Beat { std::atomic<uint32_t> id; std::atomic<int64_t> lastMs; } beat[kSlots];
    };
    static constexpr uint32_t kMagic = 0x5a574f31;   // "ZWO1"

    OrbitBus()
    {
        id = uint32_t(juce::Random::getSystemRandom().nextInt()) | 1u;
        auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("ZenWaveLabs.orbit");
        if (! f.existsAsFile() || f.getSize() < int64_t(sizeof(Shared)))
        {
            juce::FileOutputStream os(f);
            if (os.openedOk()) { juce::MemoryBlock zeros(sizeof(Shared) + 64, true); os.write(zeros.getData(), zeros.getSize()); os.flush(); }
        }
        map = std::make_unique<juce::MemoryMappedFile>(f, juce::MemoryMappedFile::readWrite);
        if (map->getData() == nullptr || map->getSize() < sizeof(Shared)) { map.reset(); return; }
        sh = static_cast<Shared*>(map->getData());
        uint32_t expected = 0;
        sh->magic.compare_exchange_strong(expected, kMagic);
        lastSeq = sh->freezeSeq.load();
    }
    ~OrbitBus() { if (sh && slot >= 0) { sh->beat[slot].id.store(0); } }

    bool ok() const { return sh != nullptr; }

    void publishFreeze(bool on)
    {
        if (! sh) return;
        sh->freezeOn.store(on ? 1u : 0u);
        sh->freezeFrom.store(id);
        lastSeq = sh->freezeSeq.fetch_add(1) + 1;
    }
    // True once per change published by another instance; fills `on`
    bool pollFreeze(bool& on)
    {
        if (! sh) return false;
        const uint32_t seq = sh->freezeSeq.load();
        if (seq == lastSeq) return false;
        lastSeq = seq;
        if (sh->freezeFrom.load() == id) return false;
        on = sh->freezeOn.load() != 0;
        return true;
    }

    // Call ~every 100 ms while Orbit is on; returns how many instances are alive (including this one)
    int heartbeat(bool active)
    {
        if (! sh) return 0;
        const int64_t now = juce::Time::currentTimeMillis();
        if (active)
        {
            if (slot < 0 || sh->beat[slot].id.load() != id)
            {
                slot = -1;
                for (int i = 0; i < kSlots; ++i)
                {
                    const uint32_t bid = sh->beat[i].id.load();
                    if (bid == 0 || now - sh->beat[i].lastMs.load() > 1500) { sh->beat[i].id.store(id); slot = i; break; }
                }
            }
            if (slot >= 0) sh->beat[slot].lastMs.store(now);
        }
        else if (slot >= 0) { sh->beat[slot].id.store(0); slot = -1; }
        int n = 0;
        for (int i = 0; i < kSlots; ++i) if (sh->beat[i].id.load() != 0 && now - sh->beat[i].lastMs.load() < 1500) ++n;
        return n;
    }

private:
    std::unique_ptr<juce::MemoryMappedFile> map;
    Shared* sh = nullptr;
    uint32_t id = 0, lastSeq = 0;
    int slot = -1;
};

// Processor-side glue: listens to the freeze parameter, mirrors it over the bus, applies
// remote freezes on the message thread. Add an "orbit" bool parameter with addOrbitParam().
inline void addOrbitParam(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ OrbitBus::orbitParamId, 1 }, "Orbit", false));
}

class OrbitLink : private juce::Timer, private juce::AudioProcessorValueTreeState::Listener
{
public:
    OrbitLink(juce::AudioProcessorValueTreeState& s, const char* freezeId) : apvts(s), freezeParamId(freezeId)
    {
        pOrbit = apvts.getRawParameterValue(OrbitBus::orbitParamId);
        apvts.addParameterListener(freezeParamId, this);
        startTimerHz(20);
    }
    ~OrbitLink() override { apvts.removeParameterListener(freezeParamId, this); }

    std::atomic<int> peers { 0 };   // instances in orbit (incl. this one), for the UI

private:
    void parameterChanged(const juce::String&, float v) override
    {
        if (applying || pOrbit->load() < 0.5f) return;
        bus.publishFreeze(v > 0.5f);
    }
    void timerCallback() override
    {
        const bool on = pOrbit->load() > 0.5f;
        if (++beatDiv % 2 == 0) peers.store(bus.heartbeat(on));
        if (! on) return;
        bool f;
        if (bus.pollFreeze(f))
            if (auto* p = apvts.getParameter(freezeParamId))
            {
                applying = true;
                p->beginChangeGesture(); p->setValueNotifyingHost(f ? 1.0f : 0.0f); p->endChangeGesture();
                applying = false;
            }
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::String freezeParamId;
    std::atomic<float>* pOrbit{};
    OrbitBus bus;
    bool applying = false; int beatDiv = 0;
};

} // namespace zwui
