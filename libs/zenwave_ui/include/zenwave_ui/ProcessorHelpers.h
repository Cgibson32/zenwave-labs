#pragma once
// Shared processor-side helpers: click-free power switch, meters, host transport → Motion,
// and tempo-sync division tables. JUCE-dependent, DSP-agnostic.

#include <juce_audio_processors/juce_audio_processors.h>
#include "ZenWaveUI.h"
#include <array>

namespace zwui {

// Note divisions used by every "Sync" control
struct Division { const char* name; double beats; };
inline const std::array<Division, 13>& divisions()
{
    static const std::array<Division, 13> d {{
        { "1/32", 0.125 }, { "1/16T", 1.0 / 6.0 }, { "1/16", 0.25 }, { "1/8T", 1.0 / 3.0 }, { "1/16.", 0.375 },
        { "1/8", 0.5 }, { "1/4T", 2.0 / 3.0 }, { "1/8.", 0.75 }, { "1/4", 1.0 }, { "1/2", 2.0 }, { "1/1", 4.0 },
        { "2 bars", 8.0 }, { "4 bars", 16.0 }
    }};
    return d;
}
inline juce::StringArray divisionNames() { juce::StringArray a; for (auto& d : divisions()) a.add(d.name); return a; }
inline double divisionSeconds(int index, double bpm)
{
    index = juce::jlimit(0, int(divisions().size()) - 1, index);
    return 60.0 / juce::jlimit(20.0, 300.0, bpm) * divisions()[size_t(index)].beats;
}

// Reads the host playhead into Motion (bpm, beat phase, playing)
inline double readTransport(juce::AudioProcessor& proc, Motion& m, double fallbackBpm = 120.0)
{
    double bpm = fallbackBpm;
    if (auto* ph = proc.getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = juce::jlimit(20.0, 300.0, *b);
            if (auto ppq = pos->getPpqPosition()) { m.beatPhase.store(float(*ppq - std::floor(*ppq))); m.ppq.store(*ppq); }
            m.playing.store(pos->getIsPlaying());
        }
    m.bpm.store(float(bpm));
    return bpm;
}

// Click-free bypass: crossfades between the untouched input and the processed output.
class PowerSwitch
{
public:
    void prepare(double sr, int maxBlock, int channels)
    {
        dry.setSize(channels, maxBlock);
        gain.reset(sr, 0.012);   // 12 ms
        gain.setCurrentAndTargetValue(1.0f);
    }
    // Call before processing: snapshots the dry signal
    void begin(const juce::AudioBuffer<float>& in, bool on)
    {
        gain.setTargetValue(on ? 1.0f : 0.0f);
        bypassedEntirely = (! on) && gain.getCurrentValue() <= 0.0001f;
        if (! bypassedEntirely)
            for (int c = 0; c < juce::jmin(in.getNumChannels(), dry.getNumChannels()); ++c)
                dry.copyFrom(c, 0, in, c, 0, in.getNumSamples());
    }
    bool skipProcessing() const { return bypassedEntirely; }
    // Call after processing: mixes dry back in according to the crossfade
    void end(juce::AudioBuffer<float>& out)
    {
        if (bypassedEntirely) return;
        if (gain.isSmoothing() || gain.getCurrentValue() < 1.0f)
        {
            const int n = out.getNumSamples();
            for (int i = 0; i < n; ++i)
            {
                const float g = gain.getNextValue();
                for (int c = 0; c < juce::jmin(out.getNumChannels(), dry.getNumChannels()); ++c)
                {
                    float* o = out.getWritePointer(c);
                    o[i] = o[i] * g + dry.getSample(c, i) * (1.0f - g);
                }
            }
        }
    }
private:
    juce::AudioBuffer<float> dry;
    juce::LinearSmoothedValue<float> gain;
    bool bypassedEntirely = false;
};

// Simple peak meter with release, publishing to an atomic for the UI
class Meter
{
public:
    void prepare(double sr) { release = float(std::exp(-1.0 / (0.25 * sr / 64.0))); }
    void push(const juce::AudioBuffer<float>& b, std::atomic<float>& out)
    {
        float pk = 0.0f;
        for (int c = 0; c < b.getNumChannels(); ++c) pk = juce::jmax(pk, b.getMagnitude(c, 0, b.getNumSamples()));
        held = pk > held ? pk : held * release;
        out.store(juce::jlimit(0.0f, 1.0f, held));
    }
private:
    float held = 0.0f, release = 0.9f;
};

} // namespace zwui

namespace zwui {

// Lock-free sample tap: the processor pushes (mono-summed) samples, the editor pulls
// a block and runs an FFT. Fixed 2048-point window.
class SpectrumTap
{
public:
    static constexpr int kSize = 4096;
    void push(const juce::AudioBuffer<float>& b)
    {
        const int n = b.getNumSamples();
        const float* l = b.getReadPointer(0);
        const float* r = b.getNumChannels() > 1 ? b.getReadPointer(1) : l;
        int w = writePos.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i) { ring[size_t(w)] = 0.5f * (l[i] + r[i]); w = (w + 1) & (kSize - 1); }
        writePos.store(w, std::memory_order_release);
    }
    // Copies the most recent kSize samples (oldest first)
    void snapshot(float* out) const
    {
        const int w = writePos.load(std::memory_order_acquire);
        for (int i = 0; i < kSize; ++i) out[i] = ring[size_t((w + i) & (kSize - 1))];
    }
private:
    std::array<float, kSize> ring {};
    std::atomic<int> writePos { 0 };
};

} // namespace zwui

namespace zwui {

// Stereo tap for the galaxy scope: interleaved L/R pairs.
class StereoTap
{
public:
    static constexpr int kPairs = 1024;
    void push(const juce::AudioBuffer<float>& b)
    {
        if (b.getNumChannels() < 1) return;
        const int n = b.getNumSamples();
        const float* l = b.getReadPointer(0);
        const float* r = b.getNumChannels() > 1 ? b.getReadPointer(1) : l;
        int w = writePos.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i) { L[size_t(w)] = l[i]; R[size_t(w)] = r[i]; w = (w + 1) & (kPairs - 1); }
        writePos.store(w, std::memory_order_release);
    }
    void snapshot(float* outL, float* outR) const
    {
        const int w = writePos.load(std::memory_order_acquire);
        for (int i = 0; i < kPairs; ++i) { const size_t k = size_t((w + i) & (kPairs - 1)); outL[i] = L[k]; outR[i] = R[k]; }
    }
private:
    std::array<float, kPairs> L {}, R {};
    std::atomic<int> writePos { 0 };
};


// Beat-release freeze: the Freeze button is quantised to the next beat or bar while the host
// plays (Free = instant). Call advance() per sub-block with the ppq span it covers.
struct FreezeQuantizer
{
    static constexpr auto paramId = "freezeq";
    static juce::StringArray names() { return { "Free", "Beat", "Bar" }; }
    static void addParam(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    { params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ paramId, 1 }, "Freeze Quantize", names(), 0)); }

    bool advance(bool wanted, int mode, bool playing, double ppqStart, double ppqEnd)
    {
        if (wanted == effective) { pending = false; return effective; }
        if (mode == 0 || ! playing) { effective = wanted; pending = false; return effective; }
        const double unit = mode == 1 ? 1.0 : 4.0;
        pending = true;
        if (std::floor(ppqEnd / unit + 1e-6) > std::floor(ppqStart / unit + 1e-6)) { effective = wanted; pending = false; }
        return effective;
    }
    bool effective = false, pending = false;   // pending: UI blinks the button until the boundary
};

} // namespace zwui
