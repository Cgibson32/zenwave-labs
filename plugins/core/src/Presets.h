#pragma once
#include <JuceHeader.h>

namespace Presets
{
    struct Band { int character; float drive, mix, gainDb; };
    struct Preset
    {
        const char* name;
        const char* category;
        float drive, heat, focus, width, mix, outputDb, lowX, highX;
        bool subLock;
        Band low, mid, high;
    };

    int count();
    juce::String name(int index);
    juce::String category(int index);
    const Preset& get(int index);
    void apply(int index, juce::AudioProcessorValueTreeState& apvts);
}
