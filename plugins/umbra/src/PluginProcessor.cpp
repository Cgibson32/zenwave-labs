#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static const char* kIds[13] = {
    ParamIDs::preDelay, ParamIDs::size, ParamIDs::decay, ParamIDs::dark, ParamIDs::density, ParamIDs::motion,
    ParamIDs::gravity, ParamIDs::early, ParamIDs::duck, ParamIDs::width, ParamIDs::mix, ParamIDs::output, ParamIDs::freeze
};

UmbraAudioProcessor::UmbraAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < 13; ++i) p[i] = apvts.getRawParameterValue(kIds[i]);
}

APVTS::ParameterLayout UmbraAudioProcessor::createLayout()
{
    using P   = juce::AudioParameterFloat;
    using B   = juce::AudioParameterBool;
    using Rng = juce::NormalisableRange<float>;

    auto pctAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; });
    auto pct = [&](const char* id, const char* name, float def) {
        return std::make_unique<P>(juce::ParameterID{ id, 1 }, name, Rng{ 0.0f, 1.0f, 0.001f }, def, pctAttr);
    };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::preDelay, 1 }, "Pre-Delay",
        Rng{ 0.0f, 250.0f, 0.1f, 0.5f }, 20.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " ms"; })));
    params.push_back(pct(ParamIDs::size,    "Size",    0.65f));
    params.push_back(pct(ParamIDs::decay,   "Decay",   0.60f));
    params.push_back(pct(ParamIDs::dark,    "Dark",    0.50f));
    params.push_back(pct(ParamIDs::density, "Density", 0.60f));
    params.push_back(pct(ParamIDs::motion,  "Motion",  0.30f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::gravity, 1 }, "Gravity",
        Rng{ 0.0f, 1.0f, 0.001f }, 0.40f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float v, int) { return juce::String(juce::roundToInt(40.0f + v * 260.0f)) + " Hz"; })));
    params.push_back(pct(ParamIDs::early,   "Early",   0.30f));
    params.push_back(pct(ParamIDs::duck,    "Duck",    0.00f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::width, 1 }, "Width",
        Rng{ 0.0f, 1.5f, 0.001f }, 1.0f, pctAttr));
    params.push_back(pct(ParamIDs::mix,     "Mix",     0.30f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::output, 1 }, "Output",
        Rng{ -24.0f, 12.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " dB"; })));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::freeze, 1 }, "Freeze", false));
    return { params.begin(), params.end() };
}

void UmbraAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
    updateEngineParams();
}

bool UmbraAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void UmbraAudioProcessor::updateEngineParams()
{
    zw::UmbraParams u;
    u.preDelayMs = p[0]->load();  u.size    = p[1]->load();  u.decay = p[2]->load();  u.dark  = p[3]->load();
    u.density    = p[4]->load();  u.motion  = p[5]->load();  u.gravity = p[6]->load(); u.early = p[7]->load();
    u.duck       = p[8]->load();  u.width   = p[9]->load();  u.mix   = p[10]->load(); u.output = p[11]->load();
    u.freeze     = p[12]->load() > 0.5f;
    engine.setParams(u);
}

void UmbraAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateEngineParams();
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    if (numOut >= 2)
    {
        if (numIn == 1) buffer.copyFrom(1, 0, buffer, 0, 0, n);
        engine.process(buffer.getWritePointer(0), buffer.getWritePointer(1), n);
        for (int ch = 2; ch < numOut; ++ch) buffer.clear(ch, 0, n);
    }
    else if (numOut == 1) engine.processMono(buffer.getWritePointer(0), n);
}

void UmbraAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= Presets::count()) return;
    currentPreset = index;
    Presets::apply(index, apvts);
}

void UmbraAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void UmbraAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            currentPreset = int(tree.getProperty("preset", 0));
            apvts.replaceState(tree);
        }
}

juce::AudioProcessorEditor* UmbraAudioProcessor::createEditor() { return new UmbraAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new UmbraAudioProcessor(); }
