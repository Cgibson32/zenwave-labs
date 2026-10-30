#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static const char* kIds[23] = {
    ParamIDs::preDelay, ParamIDs::size, ParamIDs::decay, ParamIDs::dark, ParamIDs::density, ParamIDs::motion,
    ParamIDs::gravity, ParamIDs::early, ParamIDs::duck, ParamIDs::width, ParamIDs::mix, ParamIDs::output, ParamIDs::freeze,
    ParamIDs::power, ParamIDs::motionSync, ParamIDs::motionDiv, ParamIDs::motionRate,
    ParamIDs::lowCut, ParamIDs::highCut, ParamIDs::lowSlope, ParamIDs::highSlope, ParamIDs::tilt, ParamIDs::bloom
};

UmbraAudioProcessor::UmbraAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < 23; ++i) p[i] = apvts.getRawParameterValue(kIds[i]);
    mod.attach(apvts, umbraModTargets());
    pad.attach(apvts);
    bank.setAlwaysLocked({ zwui::PadIDs::x, zwui::PadIDs::y, zwui::PadIDs::hold, zwui::PadIDs::glide });
    bank.setAlwaysLocked({ ParamIDs::power, ParamIDs::output, ParamIDs::mix, ParamIDs::freeze, zwui::OrbitBus::orbitParamId });
    bank.setSafeRange(ParamIDs::lowCut, 20.0f, 400.0f);
    bank.setSafeRange(ParamIDs::highCut, 2500.0f, 20000.0f);
    bank.setSafeRange(ParamIDs::width, 0.6f, 1.5f);
    bank.setSafeRange(ParamIDs::duck, 0.0f, 0.7f);
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
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::power, 1 }, "Power", true));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::motionSync, 1 }, "Motion Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ ParamIDs::motionDiv, 1 }, "Motion Division", zwui::divisionNames(), 11));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::motionRate, 1 }, "Motion Rate",
        Rng{ 0.1f, 4.0f, 0.001f, 0.5f }, 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 2) + "x"; })));
    auto hzAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float v, int) { return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; });
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::lowCut, 1 }, "Low Cut", Rng{ 20.0f, 2000.0f, 0.1f, 0.35f }, 20.0f, hzAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::highCut, 1 }, "High Cut", Rng{ 1000.0f, 20000.0f, 1.0f, 0.5f }, 20000.0f, hzAttr));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ ParamIDs::lowSlope, 1 }, "Low Slope", juce::StringArray{ "6 dB", "12 dB", "24 dB", "48 dB" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ ParamIDs::highSlope, 1 }, "High Slope", juce::StringArray{ "6 dB", "12 dB", "24 dB", "48 dB" }, 1));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::tilt, 1 }, "Tilt", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(v * 6.0f, 1) + " dB"; })));
    params.push_back(pct(ParamIDs::bloom, "Bloom", 0.0f));
    zwui::addModParams(params, umbraModTargets());
    zwui::addOrbitParam(params);
    zwui::addPadParams(params);
    return { params.begin(), params.end() };
}

void UmbraAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare(sampleRate, samplesPerBlock);
    mod.prepare(sampleRate);
    pad.prepare(sampleRate);
    wetScratch.setSize(2, samplesPerBlock);
    power.prepare(sampleRate, samplesPerBlock, 2);
    meterIn.prepare(sampleRate); meterOut.prepare(sampleRate);
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
    const auto M = [this](int t) { return mod.value(t); };
    float decay = M(ModT::Decay), mix = M(ModT::Mix);
    const float pa = pad.eased.load();
    if (pa > 0.0001f) { decay = juce::jmap(pa, decay, pad.x()); mix = juce::jmap(pa, mix, pad.y()); }
    pad.effX.store(decay); pad.effY.store(mix);
    u.preDelayMs = M(ModT::PreDelay); u.size = M(ModT::Size);     u.decay = decay;          u.dark  = M(ModT::Dark);
    u.density    = M(ModT::Density);  u.motion = M(ModT::Motion); u.gravity = M(ModT::Gravity); u.early = M(ModT::Early);
    u.duck       = M(ModT::Duck);     u.width = M(ModT::Width);   u.mix   = mix;            u.output = p[11]->load();
    u.freeze     = p[12]->load() > 0.5f;
    // Motion sync: slowest reverb LFO (0.09 Hz base) locks to one cycle per division
    u.motionRate = p[14]->load() > 0.5f
                     ? float((1.0 / zwui::divisionSeconds(int(p[15]->load()), hostBpm)) / 0.09)
                     : M(ModT::MotionRate);
    u.lowCutHz = M(ModT::LowCut); u.highCutHz = M(ModT::HighCut);
    u.lowSlope = int(p[19]->load()); u.highSlope = int(p[20]->load()); u.tilt = M(ModT::Tilt);
    u.bloom = M(ModT::Bloom);
    engine.setParams(u);
}

void UmbraAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    hostBpm = zwui::readTransport(*this, motion, hostBpm);
    meterIn.push(buffer, motion.levelIn);
    spectrumIn.push(buffer);
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    power.begin(buffer, p[13]->load() > 0.5f);
    if (power.skipProcessing()) { mod.process(buffer.getReadPointer(0), numIn > 1 ? buffer.getReadPointer(1) : nullptr, n, hostBpm, motion.ppq.load(), motion.playing.load()); updateEngineParams(); motion.levelOut.store(0.0f); return; }
    if (numOut >= 2 && numIn == 1) buffer.copyFrom(1, 0, buffer, 0, 0, n);

    constexpr int kSub = 64;
    const bool playing = motion.playing.load();
    double ppq = motion.ppq.load();
    for (int start = 0; start < n; start += kSub)
    {
        const int len = juce::jmin(kSub, n - start);
        mod.process(buffer.getReadPointer(0, start), numOut >= 2 ? buffer.getReadPointer(1, start) : nullptr, len, hostBpm, ppq, playing);
        pad.process(len, hostBpm);
        updateEngineParams();
        if (wetScratch.getNumSamples() >= n) engine.setWetTap(wetScratch.getWritePointer(0, start), wetScratch.getWritePointer(1, start));
        if (numOut >= 2) engine.process(buffer.getWritePointer(0, start), buffer.getWritePointer(1, start), len);
        else if (numOut == 1) engine.processMono(buffer.getWritePointer(0, start), len);
        engine.setWetTap(nullptr, nullptr);
        if (playing) ppq += len / currentSampleRate * hostBpm / 60.0;
    }
    for (int ch = 2; ch < numOut; ++ch) buffer.clear(ch, 0, n);
    power.end(buffer);
    meterOut.push(buffer, motion.levelOut);
    if (wetScratch.getNumSamples() >= n && numOut >= 2) { juce::AudioBuffer<float> wet(wetScratch.getArrayOfWritePointers(), 2, n); spectrum.push(wet); }
    else spectrum.push(buffer);
    stereo.push(buffer);
}

void UmbraAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= Presets::count()) return;
    currentPreset = index;
    zwui::resetModParams(apvts);
    Presets::apply(index, apvts);
    bank.clearCurrentUser();
}

void UmbraAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    bank.writeTo(state);
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
            bank.readFrom(tree);
        }
}

juce::AudioProcessorEditor* UmbraAudioProcessor::createEditor() { return new UmbraAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new UmbraAudioProcessor(); }
