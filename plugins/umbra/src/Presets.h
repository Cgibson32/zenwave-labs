#pragma once
#include <JuceHeader.h>

namespace Presets
{
    struct Preset
    {
        const char* name;
        const char* category;
        float preDelayMs, size, decay, dark, density, motion, gravity, early, duck, width, mix, outputDb;
        bool freeze;
    };

    int count();
    juce::String name(int index);
    juce::String category(int index);
    const Preset& get(int index);
    void apply(int index, juce::AudioProcessorValueTreeState& apvts);
}
