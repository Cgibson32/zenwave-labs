#pragma once
#include <JuceHeader.h>

// Factory presets for Drift. Values are normalised 0..1 except where noted (see Presets.cpp).
namespace Presets
{
    struct Preset
    {
        const char* name;
        const char* category;
        float timeMs; bool sync; int division;
        float feedback, wobble, age, space, size, decay, width, spread;
        bool pingPong;
        float mix, outputDb;
    };

    int count();
    juce::String name(int index);
    juce::String category(int index);
    const Preset& get(int index);
    void apply(int index, juce::AudioProcessorValueTreeState& apvts);
}
