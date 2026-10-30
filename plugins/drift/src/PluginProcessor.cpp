#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;


DriftAudioProcessor::DriftAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    pTime     = apvts.getRawParameterValue(ParamIDs::time);
    pSync     = apvts.getRawParameterValue(ParamIDs::sync);
    pDivision = apvts.getRawParameterValue(ParamIDs::division);
    pFeedback = apvts.getRawParameterValue(ParamIDs::feedback);
    pWow      = apvts.getRawParameterValue(ParamIDs::wowFlutter);
    pAge      = apvts.getRawParameterValue(ParamIDs::age);
    pSpace    = apvts.getRawParameterValue(ParamIDs::space);
    pSize     = apvts.getRawParameterValue(ParamIDs::size);
    pDecay    = apvts.getRawParameterValue(ParamIDs::decay);
    pWidth    = apvts.getRawParameterValue(ParamIDs::width);
    pSpread   = apvts.getRawParameterValue(ParamIDs::spread);
    pPingPong = apvts.getRawParameterValue(ParamIDs::pingPong);
    pFreeze   = apvts.getRawParameterValue(ParamIDs::freeze);
    pMix      = apvts.getRawParameterValue(ParamIDs::mix);
    pOutput   = apvts.getRawParameterValue(ParamIDs::output);
    pPower    = apvts.getRawParameterValue(ParamIDs::power);
    pWobbleSync = apvts.getRawParameterValue(ParamIDs::wobbleSync);
    pWobbleDiv  = apvts.getRawParameterValue(ParamIDs::wobbleDiv);
    pWobbleRate = apvts.getRawParameterValue(ParamIDs::wobbleRate);
    pLowCut = apvts.getRawParameterValue(ParamIDs::lowCut);
    pHighCut = apvts.getRawParameterValue(ParamIDs::highCut);
    pLowSlope = apvts.getRawParameterValue(ParamIDs::lowSlope);
    pHighSlope = apvts.getRawParameterValue(ParamIDs::highSlope);
    pTilt = apvts.getRawParameterValue(ParamIDs::tilt);
}

APVTS::ParameterLayout DriftAudioProcessor::createLayout()
{
    using P   = juce::AudioParameterFloat;
    using B   = juce::AudioParameterBool;
    using C   = juce::AudioParameterChoice;
    using Rng = juce::NormalisableRange<float>;

    auto pct = [](const char* id, const char* name, float def) {
        return std::make_unique<P>(juce::ParameterID{ id, 1 }, name, Rng{ 0.0f, 1.0f, 0.001f }, def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; }));
    };

    const auto divNames = zwui::divisionNames();

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::time, 1 }, "Time",
        Rng{ 20.0f, 2000.0f, 0.1f, 0.4f }, 380.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float v, int) { return juce::String(v, v < 100.0f ? 1 : 0) + " ms"; })));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::sync, 1 }, "Sync", false));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::division, 1 }, "Division", divNames, 5));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::power, 1 }, "Power", true));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::wobbleSync, 1 }, "Wobble Sync", false));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::wobbleDiv, 1 }, "Wobble Division", divNames, 10));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::wobbleRate, 1 }, "Wobble Rate",
        Rng{ 0.05f, 4.0f, 0.001f, 0.5f }, 0.45f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 2) + " Hz"; })));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::feedback, 1 }, "Feedback",
        Rng{ 0.0f, 1.1f, 0.001f }, 0.45f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; })));
    params.push_back(pct(ParamIDs::wowFlutter, "Wobble", 0.30f));
    params.push_back(pct(ParamIDs::age,        "Age",    0.40f));
    params.push_back(pct(ParamIDs::space,      "Space",  0.35f));
    params.push_back(pct(ParamIDs::size,       "Size",   0.60f));
    params.push_back(pct(ParamIDs::decay,      "Decay",  0.60f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::width, 1 }, "Width",
        Rng{ 0.0f, 1.5f, 0.001f }, 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; })));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::spread, 1 }, "Spread",
        Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float v, int) { return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v * 100.0f)) + " %"; })));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::pingPong, 1 }, "Ping-Pong", true));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::freeze, 1 }, "Freeze", false));
    params.push_back(pct(ParamIDs::mix, "Mix", 0.35f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::output, 1 }, "Output",
        Rng{ -24.0f, 12.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float v, int) { return juce::String(v, 1) + " dB"; })));
    auto hzAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float v, int) { return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; });
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::lowCut, 1 }, "Low Cut", Rng{ 20.0f, 2000.0f, 0.1f, 0.35f }, 20.0f, hzAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::highCut, 1 }, "High Cut", Rng{ 1000.0f, 20000.0f, 1.0f, 0.5f }, 20000.0f, hzAttr));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::lowSlope, 1 }, "Low Slope", juce::StringArray{ "6 dB", "12 dB", "24 dB", "48 dB" }, 1));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::highSlope, 1 }, "High Slope", juce::StringArray{ "6 dB", "12 dB", "24 dB", "48 dB" }, 1));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::tilt, 1 }, "Tilt", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(v * 6.0f, 1) + " dB"; })));
    return { params.begin(), params.end() };
}

void DriftAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare(sampleRate, samplesPerBlock);
    monoScratch.setSize(1, samplesPerBlock);
    power.prepare(sampleRate, samplesPerBlock, 2);
    meterIn.prepare(sampleRate); meterOut.prepare(sampleRate);
    updateEngineParams();
}

bool DriftAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

float DriftAudioProcessor::syncedTimeMs() const
{
    return float(zwui::divisionSeconds(int(pDivision->load()), hostBpm) * 1000.0);
}

void DriftAudioProcessor::updateEngineParams()
{
    zw::DriftParams p;
    p.timeMs     = pSync->load() > 0.5f ? syncedTimeMs() : pTime->load();
    p.feedback   = pFeedback->load();
    p.wowFlutter = pWow->load();
    p.age        = pAge->load();
    p.space      = pSpace->load();
    p.size       = pSize->load();
    p.decay      = pDecay->load();
    p.width      = pWidth->load();
    p.spread     = pSpread->load();
    p.pingPong   = pPingPong->load() > 0.5f;
    p.freeze     = pFreeze->load() > 0.5f;
    p.mix        = pMix->load();
    p.output     = pOutput->load();
    p.wobbleHz   = pWobbleSync->load() > 0.5f
                     ? float(1.0 / zwui::divisionSeconds(int(pWobbleDiv->load()), hostBpm))
                     : pWobbleRate->load();
    p.lowCutHz  = pLowCut->load();  p.highCutHz = pHighCut->load();
    p.lowSlope  = int(pLowSlope->load()); p.highSlope = int(pHighSlope->load());
    p.tilt      = pTilt->load();
    engine.setParams(p);
}

void DriftAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    hostBpm = zwui::readTransport(*this, motion, hostBpm);
    updateEngineParams();
    meterIn.push(buffer, motion.levelIn);

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    power.begin(buffer, pPower->load() > 0.5f);
    if (power.skipProcessing()) { motion.levelOut.store(0.0f); return; }

    if (numOut >= 2)
    {
        if (numIn == 1) buffer.copyFrom(1, 0, buffer, 0, 0, n);
        engine.process(buffer.getWritePointer(0), buffer.getWritePointer(1), n);
        for (int ch = 2; ch < numOut; ++ch) buffer.clear(ch, 0, n);
    }
    else if (numOut == 1)
    {
        engine.processMono(buffer.getWritePointer(0), n);
    }

    power.end(buffer);
    meterOut.push(buffer, motion.levelOut);
    spectrum.push(buffer);
}

void DriftAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= Presets::count()) return;
    currentPreset = index;
    Presets::apply(index, apvts);
}

void DriftAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void DriftAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            currentPreset = int(tree.getProperty("preset", 0));
            apvts.replaceState(tree);
        }
}

juce::AudioProcessorEditor* DriftAudioProcessor::createEditor()
{
    return new DriftAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DriftAudioProcessor();
}
