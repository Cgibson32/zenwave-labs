#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

PulseAudioProcessor::PulseAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    pOutput = apvts.getRawParameterValue(ParamIDs::output);
    pIn = apvts.getRawParameterValue(ParamIDs::in);
    pAuto = apvts.getRawParameterValue(ParamIDs::autoMode);
    pSubLock = apvts.getRawParameterValue(ParamIDs::subLock);
    pGrit = apvts.getRawParameterValue(ParamIDs::grit);
    pPower = apvts.getRawParameterValue(ParamIDs::power);
    pScAmount = apvts.getRawParameterValue(ParamIDs::scAmount);
    pScRelease = apvts.getRawParameterValue(ParamIDs::scRelease);
    for (int b = 0; b < 3; ++b)
    {
        pBandOn[b] = apvts.getRawParameterValue(ParamIDs::bandOn(b));   pBandSolo[b] = apvts.getRawParameterValue(ParamIDs::bandSolo(b));
        pBandGain[b] = apvts.getRawParameterValue(ParamIDs::bandGain(b)); pBandAtk[b] = apvts.getRawParameterValue(ParamIDs::bandAtk(b));
        pBandRel[b] = apvts.getRawParameterValue(ParamIDs::bandRel(b));
    }
    for (int i = 0; i < 4; ++i) { pNodeOn[i] = apvts.getRawParameterValue(ParamIDs::nodeOn(i)); pNodeQ[i] = apvts.getRawParameterValue(ParamIDs::nodeQ(i)); pNodeThr[i] = apvts.getRawParameterValue(ParamIDs::nodeThr(i)); }
    mod.attach(apvts, pulseModTargets());
    pad.attach(apvts);
    bank.setAlwaysLocked({ ParamIDs::power, ParamIDs::output, ParamIDs::in, ParamIDs::mix, ParamIDs::subLock, ParamIDs::scAmount,
                           zwui::PadIDs::x, zwui::PadIDs::y, zwui::PadIDs::hold, zwui::PadIDs::glide });
    for (int b = 0; b < 3; ++b) { bank.setAlwaysLocked({ ParamIDs::bandOn(b).toRawUTF8(), ParamIDs::bandSolo(b).toRawUTF8() }); bank.setSafeRange(ParamIDs::bandGain(b).toRawUTF8(), -6.0f, 6.0f); }
    for (int i = 0; i < 4; ++i) bank.setSafeRange(ParamIDs::nodeRange(i).toRawUTF8(), -12.0f, 6.0f);
    bank.setSafeRange(ParamIDs::lowX, 60.0f, 300.0f); bank.setSafeRange(ParamIDs::highX, 1500.0f, 8000.0f);
    bank.setSafeRange(ParamIDs::duck, 0.0f, 0.5f);
}

APVTS::ParameterLayout PulseAudioProcessor::createLayout()
{
    using P   = juce::AudioParameterFloat;
    using B   = juce::AudioParameterBool;
    using Rng = juce::NormalisableRange<float>;

    auto pctAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; });
    auto dbAttr  = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB"; });
    auto hzAttr  = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; });
    auto msAttr  = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v)) + " ms"; });
    auto mulAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 2) + "x"; });
    auto pct = [&](const juce::String& id, const juce::String& name, float def) { return std::make_unique<P>(juce::ParameterID{ id, 1 }, name, Rng{ 0.0f, 1.0f, 0.001f }, def, pctAttr); };
    auto bip = [&](const juce::String& id, const juce::String& name, float def) { return std::make_unique<P>(juce::ParameterID{ id, 1 }, name, Rng{ -1.0f, 1.0f, 0.001f }, def,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v * 100)) + " %"; })); };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::in, 1 }, "Input", Rng{ -24.0f, 24.0f, 0.1f }, 0.0f, dbAttr));
    params.push_back(pct(ParamIDs::depth, "Depth", 0.5f));
    params.push_back(pct(ParamIDs::time,  "Time",  0.5f));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::autoMode, 1 }, "Auto", true));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::subLock, 1 }, "Sub Lock", true));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::grit, 1 }, "Grit", false));
    params.push_back(bip(ParamIDs::shapeAtk, "Attack", 0.0f));
    params.push_back(bip(ParamIDs::shapeSus, "Sustain", 0.0f));
    params.push_back(pct(ParamIDs::duck, "Duck", 0.0f));
    params.push_back(pct(ParamIDs::scAmount, "Sidechain", 0.0f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::scRelease, 1 }, "SC Release", Rng{ 20.0f, 1000.0f, 1.0f, 0.5f }, 150.0f, msAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::lowX, 1 }, "Low X", Rng{ 40.0f, 400.0f, 0.1f, 0.5f }, 150.0f, hzAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::highX, 1 }, "High X", Rng{ 1000.0f, 12000.0f, 1.0f, 0.5f }, 3000.0f, hzAttr));
    params.push_back(pct(ParamIDs::mix, "Mix", 1.0f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::output, 1 }, "Output", Rng{ -24.0f, 12.0f, 0.1f }, 0.0f, dbAttr));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::power, 1 }, "Power", true));
    const char* bandNames[3] = { "Low", "Mid", "High" };
    for (int b = 0; b < 3; ++b)
    {
        const juce::String n(bandNames[b]);
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::bandOn(b), 1 }, n + " On", true));
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::bandSolo(b), 1 }, n + " Solo", false));
        params.push_back(pct(ParamIDs::bandDepth(b), n + " Depth", 1.0f));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::bandGain(b), 1 }, n + " Gain", Rng{ -18.0f, 18.0f, 0.1f }, 0.0f, dbAttr));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::bandAtk(b), 1 }, n + " Attack", Rng{ 0.25f, 4.0f, 0.01f, 0.5f }, 1.0f, mulAttr));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::bandRel(b), 1 }, n + " Release", Rng{ 0.25f, 4.0f, 0.01f, 0.5f }, 1.0f, mulAttr));
    }
    const float nodeDef[4] = { 250.0f, 800.0f, 3000.0f, 8000.0f };
    for (int i = 0; i < 4; ++i)
    {
        const juce::String n = "Node " + juce::String(i + 1) + " ";
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::nodeOn(i), 1 }, n + "On", false));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::nodeFreq(i), 1 }, n + "Freq", Rng{ 20.0f, 20000.0f, 0.1f, 0.25f }, nodeDef[i], hzAttr));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::nodeQ(i), 1 }, n + "Width", Rng{ 0.3f, 12.0f, 0.01f, 0.4f }, 2.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return "Q " + juce::String(v, 1); })));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::nodeThr(i), 1 }, n + "Threshold", Rng{ -24.0f, 24.0f, 0.1f }, 0.0f, dbAttr));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::nodeRange(i), 1 }, n + "Range", Rng{ -18.0f, 18.0f, 0.1f }, -6.0f, dbAttr));
    }
    zwui::addModParams(params, pulseModTargets());
    zwui::addPadParams(params);
    return { params.begin(), params.end() };
}

void PulseAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare(sampleRate, samplesPerBlock);
    setLatencySamples(engine.latencySamples());
    mod.prepare(sampleRate);
    pad.prepare(sampleRate);
    wetScratch.setSize(2, samplesPerBlock);
    power.prepare(sampleRate, samplesPerBlock, 2);
    meterIn.prepare(sampleRate); meterOut.prepare(sampleRate);
    updateEngineParams();
}

bool PulseAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (! (in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo()))) return false;
    const auto& sc = layouts.getChannelSet(true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void PulseAudioProcessor::updateEngineParams()
{
    zw::PulseParams c;
    const auto M = [this](int t) { return mod.value(t); };
    float depth = M(ModT::Depth), duck = M(ModT::Duck);
    const float pa = pad.eased.load();
    if (pa > 0.0001f) { depth = juce::jmap(pa, depth, pad.x()); duck = juce::jmap(pa, duck, pad.y()); }
    pad.effX.store(depth); pad.effY.store(duck);
    c.inGain = pIn->load(); c.depth = depth; c.time = M(ModT::Time); c.mix = M(ModT::Mix); c.duck = duck;
    c.shapeAttack = M(ModT::ShapeAtk); c.shapeSustain = M(ModT::ShapeSus);
    c.lowX = M(ModT::LowX); c.highX = juce::jmax(M(ModT::HighX), c.lowX * 2.5f);
    c.autoMode = pAuto->load() > 0.5f; c.subLock = pSubLock->load() > 0.5f; c.grit = pGrit->load() > 0.5f;
    c.scAmount = pScAmount->load(); c.scReleaseMs = pScRelease->load(); c.output = pOutput->load();
    const int depthT[3] = { ModT::LowDepth, ModT::MidDepth, ModT::HighDepth };
    for (int b = 0; b < 3; ++b)
    {
        c.band[b].enabled = pBandOn[b]->load() > 0.5f; c.band[b].solo = pBandSolo[b]->load() > 0.5f;
        c.band[b].depth = M(depthT[b]); c.band[b].gain = pBandGain[b]->load();
        c.band[b].attack = pBandAtk[b]->load(); c.band[b].release = pBandRel[b]->load();
    }
    const int fT[4] = { ModT::N1Freq, ModT::N2Freq, ModT::N3Freq, ModT::N4Freq }, rT[4] = { ModT::N1Range, ModT::N2Range, ModT::N3Range, ModT::N4Range };
    for (int i = 0; i < 4; ++i)
    {
        c.node[i].on = pNodeOn[i]->load() > 0.5f; c.node[i].freqHz = M(fT[i]); c.node[i].q = pNodeQ[i]->load();
        c.node[i].thresh = pNodeThr[i]->load(); c.node[i].range = M(rT[i]);
    }
    engine.setParams(c);
}

void PulseAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    hostBpm = zwui::readTransport(*this, motion, hostBpm);
    auto main = getBusBuffer(buffer, true, 0);
    meterIn.push(main, motion.levelIn);
    spectrumIn.push(main);
    const int n = buffer.getNumSamples();
    const int numIn = main.getNumChannels(), numOut = getTotalNumOutputChannels();

    const float* scL = nullptr; const float* scR = nullptr;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled())
    {
        auto sc = getBusBuffer(buffer, true, 1);
        if (sc.getNumChannels() > 0) { scL = sc.getReadPointer(0); scR = sc.getNumChannels() > 1 ? sc.getReadPointer(1) : scL; }
    }
    sidechainPresent.store(scL != nullptr);

    // in/out loudness for MATCH
    { double e = 0; for (int c = 0; c < juce::jmin(2, numIn); ++c) { const float* d = main.getReadPointer(c); for (int i = 0; i < n; ++i) e += d[i] * d[i]; } inRms.store(float(std::sqrt(e / juce::jmax(1, n * juce::jmin(2, numIn))))); }

    power.begin(buffer, pPower->load() > 0.5f);
    if (power.skipProcessing()) { mod.process(main.getReadPointer(0), numIn > 1 ? main.getReadPointer(1) : nullptr, n, hostBpm, motion.ppq.load(), motion.playing.load()); updateEngineParams(); motion.levelOut.store(0.0f); return; }
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
        if (numOut >= 2) engine.process(buffer.getWritePointer(0, start), buffer.getWritePointer(1, start), scL ? scL + start : nullptr, scR ? scR + start : nullptr, len);
        else if (numOut == 1) engine.processMono(buffer.getWritePointer(0, start), len);
        engine.setWetTap(nullptr, nullptr);
        if (playing) ppq += len / currentSampleRate * hostBpm / 60.0;
    }
    for (int ch = 2; ch < numOut; ++ch) buffer.clear(ch, 0, n);
    for (int b = 0; b < 3; ++b) bandGr[size_t(b)].store(engine.grVis[b]);
    for (int i = 0; i < 4; ++i) nodeGain[size_t(i)].store(engine.nodeGainDb(i));
    transientVis.store(engine.transientVis); duckVis.store(engine.duckVis);
    { double e = 0; for (int c = 0; c < juce::jmin(2, numOut); ++c) { const float* d = buffer.getReadPointer(c); for (int i = 0; i < n; ++i) e += d[i] * d[i]; } outRms.store(float(std::sqrt(e / juce::jmax(1, n * juce::jmin(2, numOut))))); }
    power.end(buffer);
    meterOut.push(buffer, motion.levelOut);
    if (wetScratch.getNumSamples() >= n && numOut >= 2) { juce::AudioBuffer<float> wet(wetScratch.getArrayOfWritePointers(), 2, n); spectrum.push(wet); }
    else spectrum.push(buffer);
    stereo.push(buffer);
}

void PulseAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= Presets::count()) return;
    currentPreset = index;
    zwui::resetModParams(apvts);
    Presets::apply(index, apvts);
    bank.clearCurrentUser();
}

void PulseAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    bank.writeTo(state);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void PulseAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
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

juce::AudioProcessorEditor* PulseAudioProcessor::createEditor() { return new PulseAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PulseAudioProcessor(); }
