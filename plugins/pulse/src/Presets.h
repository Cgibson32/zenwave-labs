#pragma once
#include <JuceHeader.h>

namespace Presets
{
    struct Band { float depth, gainDb, attack, release; };
    struct Node { bool on; float freqHz, q, thresh, range; };
    struct Preset
    {
        const char* name;
        const char* category;
        float inDb, depth, time, shapeAttack, shapeSustain, duck, mix, outDb, lowX, highX;
        bool autoMode, subLock, grit;
        Band low, mid, high;
        Node n1, n2;
    };

    int count();
    juce::String name(int index);
    juce::String category(int index);
    const Preset& get(int index);
    void apply(int index, juce::AudioProcessorValueTreeState& apvts);
}
