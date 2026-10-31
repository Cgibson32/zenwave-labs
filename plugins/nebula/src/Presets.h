#pragma once
#include <JuceHeader.h>

namespace Presets
{
    struct Osc { int table; float position, level; int octave, semis; float fine; int unison; float spread, width; bool on; };
    struct Preset
    {
        const char* name;
        const char* category;
        Osc a, b;
        float cutoff, res; int filterMode; float envAmt, drive, drift, glideMs;
        float ampA, ampD, ampS, ampR;
        float modA, modD, modS, modR;
        float spaceMix, spaceSize, spaceDecay, spaceDark;
        float warmth, air, movement, space, grit, width, attack, release;
    };

    int count();
    juce::String name(int index);
    juce::String category(int index);
    const Preset& get(int index);
    void apply(int index, juce::AudioProcessorValueTreeState& apvts);
}
