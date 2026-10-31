#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

CoreAudioProcessor::CoreAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    pOutput = apvts.getRawParameterValue(ParamIDs::output);
    pSubLock = apvts.getRawParameterValue(ParamIDs::subLock);
    pPower = apvts.getRawParameterValue(ParamIDs::power);
    pLowSlope = apvts.getRawParameterValue(ParamIDs::lowSlope);
    pHighSlope = apvts.getRawParameterValue(ParamIDs::highSlope);
    for (int b = 0; b < 3; ++b)
    {
        pBandChar[b] = apvts.getRawParameterValue(ParamIDs::bandChar(b));
        pBandMix[b]  = apvts.getRawParameterValue(ParamIDs::bandMix(b));
        pBandGain[b] = apvts.getRawParameterValue(ParamIDs::bandGain(b));
        pBandOn[b]   = apvts.getRawParameterValue(ParamIDs::bandOn(b));
        pBandSolo[b] = apvts.getRawParameterValue(ParamIDs::bandSolo(b));
    }
    pCrushOn = apvts.getRawParameterValue(ParamIDs::crushOn);
    pCrushMix = apvts.getRawParameterValue(ParamIDs::crushMix);
    pVowel = apvts.getRawParameterValue(ParamIDs::vowel);
    mod.attach(apvts, coreModTargets());
    pad.attach(apvts);
    bank.setAlwaysLocked({ ParamIDs::power, ParamIDs::output, ParamIDs::mix, ParamIDs::subLock,
                           zwui::PadIDs::x, zwui::PadIDs::y, zwui::PadIDs::hold, zwui::PadIDs::glide });
    bank.setSafeRange(ParamIDs::lowX, 60.0f, 300.0f);
    bank.setSafeRange(ParamIDs::highX, 1500.0f, 8000.0f);
    bank.setSafeRange(ParamIDs::width, 0.7f, 1.4f);
    bank.setSafeRange(ParamIDs::lowCut, 20.0f, 200.0f);
    bank.setSafeRange(ParamIDs::highCut, 5000.0f, 20000.0f);
    for (int b = 0; b < 3; ++b) { bank.setSafeRange(ParamIDs::bandGain(b).toRawUTF8(), -6.0f, 6.0f); bank.setSafeRange(ParamIDs::bandRes(b).toRawUTF8(), 0.0f, 0.7f); }
    for (int b = 0; b < 3; ++b) bank.setAlwaysLocked({ ParamIDs::bandOn(b).toRawUTF8(), ParamIDs::bandSolo(b).toRawUTF8() });
    bank.setSafeRange(ParamIDs::crushBits, 5.0f, 16.0f); bank.setSafeRange(ParamIDs::crushRate, 0.0f, 0.6f);
}

APVTS::ParameterLayout CoreAudioProcessor::createLayout()
{
    using P   = juce::AudioParameterFloat;
    using B   = juce::AudioParameterBool;
    using C   = juce::AudioParameterChoice;
    using Rng = juce::NormalisableRange<float>;

    auto pctAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; });
    auto dbAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float v, int) { return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB"; });
    auto hzAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float v, int) { return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; });
    auto pct = [&](const juce::String& id, const juce::String& name, float def) {
        return std::make_unique<P>(juce::ParameterID{ id, 1 }, name, Rng{ 0.0f, 1.0f, 0.001f }, def, pctAttr); };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(pct(ParamIDs::drive, "Drive", 0.40f));
    params.push_back(pct(ParamIDs::heat,  "Heat",  0.30f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::focus, 1 }, "Focus", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return v < -0.01f ? "Mid " + juce::String(juce::roundToInt(-v * 100)) + " %" : v > 0.01f ? "Side " + juce::String(juce::roundToInt(v * 100)) + " %" : juce::String("Both"); })));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::width, 1 }, "Width", Rng{ 0.0f, 1.5f, 0.001f }, 1.0f, pctAttr));
    params.push_back(pct(ParamIDs::mix, "Mix", 1.0f));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::output, 1 }, "Output", Rng{ -24.0f, 12.0f, 0.1f }, 0.0f, dbAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::lowX, 1 }, "Low X", Rng{ 40.0f, 400.0f, 0.1f, 0.5f }, 150.0f, hzAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::highX, 1 }, "High X", Rng{ 1000.0f, 12000.0f, 1.0f, 0.5f }, 3500.0f, hzAttr));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::subLock, 1 }, "Sub Lock", true));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::power, 1 }, "Power", true));
    const juce::StringArray chars { "Tape", "Tube", "Fold", "Rectify", "Crush", "Diode", "Octave", "Hard" };
    const char* bandNames[3] = { "Low", "Mid", "High" };
    for (int b = 0; b < 3; ++b)
    {
        params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::bandChar(b), 1 }, juce::String(bandNames[b]) + " Character", chars, 0));
        params.push_back(pct(ParamIDs::bandDrive(b), juce::String(bandNames[b]) + " Drive", 0.30f));
        params.push_back(pct(ParamIDs::bandMix(b), juce::String(bandNames[b]) + " Mix", 1.0f));
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::bandGain(b), 1 }, juce::String(bandNames[b]) + " Gain", Rng{ -18.0f, 18.0f, 0.1f }, 0.0f, dbAttr));
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::bandOn(b), 1 }, juce::String(bandNames[b]) + " On", true));
        params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::bandSolo(b), 1 }, juce::String(bandNames[b]) + " Solo", false));
        const float tuneDef[3] = { 80.0f, 800.0f, 6000.0f };
        params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::bandTune(b), 1 }, juce::String(bandNames[b]) + " Tune", Rng{ 20.0f, 20000.0f, 0.1f, 0.25f }, tuneDef[b], hzAttr));
        params.push_back(pct(ParamIDs::bandRes(b), juce::String(bandNames[b]) + " Resonance", 0.0f));
    }
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::crushOn, 1 }, "Crush", false));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::crushBits, 1 }, "Crush Bits", Rng{ 2.0f, 16.0f, 0.01f }, 12.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " bit"; })));
    params.push_back(pct(ParamIDs::crushRate, "Crush Rate", 0.0f));
    params.push_back(pct(ParamIDs::crushMix, "Crush Mix", 1.0f));
    params.push_back(std::make_unique<B>(juce::ParameterID{ ParamIDs::vowel, 1 }, "Vowel", false));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::punch, 1 }, "Punch", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return v < -0.01f ? "Keep hits " + juce::String(juce::roundToInt(-v * 100)) + " %" : v > 0.01f ? "Hit only " + juce::String(juce::roundToInt(v * 100)) + " %" : juce::String("Off"); })));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::lowCut, 1 }, "Low Cut", Rng{ 20.0f, 2000.0f, 0.1f, 0.35f }, 20.0f, hzAttr));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::highCut, 1 }, "High Cut", Rng{ 1000.0f, 20000.0f, 1.0f, 0.5f }, 20000.0f, hzAttr));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::lowSlope, 1 }, "Low Slope", juce::StringArray{ "6 dB", "12 dB", "24 dB", "48 dB" }, 1));
    params.push_back(std::make_unique<C>(juce::ParameterID{ ParamIDs::highSlope, 1 }, "High Slope", juce::StringArray{ "6 dB", "12 dB", "24 dB", "48 dB" }, 1));
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::tilt, 1 }, "Tilt", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return (v > 0 ? "+" : "") + juce::String(v * 6.0f, 1) + " dB"; })));
    zwui::addModParams(params, coreModTargets());
    zwui::addPadParams(params);
    return { params.begin(), params.end() };
}

void CoreAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
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

bool CoreAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void CoreAudioProcessor::updateEngineParams()
{
    zw::CoreParams c;
    const auto M = [this](int t) { return mod.value(t); };
    float drive = M(ModT::Drive), mix = M(ModT::Mix);
    const float pa = pad.eased.load();
    if (pa > 0.0001f) { drive = juce::jmap(pa, drive, pad.x()); mix = juce::jmap(pa, mix, pad.y()); }
    pad.effX.store(drive); pad.effY.store(mix);
    c.drive = drive; c.heat = M(ModT::Heat); c.focus = M(ModT::Focus); c.width = M(ModT::Width); c.mix = mix;
    c.output = pOutput->load();
    c.lowX = M(ModT::LowX); c.highX = juce::jmax(M(ModT::HighX), c.lowX * 2.5f);
    c.subLock = pSubLock->load() > 0.5f;
    const int driveT[3] = { ModT::LowDrive, ModT::MidDrive, ModT::HighDrive }, tuneT[3] = { ModT::LowTune, ModT::MidTune, ModT::HighTune }, resT[3] = { ModT::LowRes, ModT::MidRes, ModT::HighRes };
    for (int b = 0; b < 3; ++b)
    {
        c.band[b].character = int(pBandChar[b]->load());
        c.band[b].drive = M(driveT[b]);
        c.band[b].mix = pBandMix[b]->load();
        c.band[b].gain = pBandGain[b]->load();
        c.band[b].enabled = pBandOn[b]->load() > 0.5f;
        c.band[b].solo = pBandSolo[b]->load() > 0.5f;
        c.band[b].tuneHz = M(tuneT[b]);
        c.band[b].res = M(resT[b]);
    }
    c.vowel = pVowel->load() > 0.5f; c.punch = M(ModT::Punch);
    c.crush.on = pCrushOn->load() > 0.5f; c.crush.bits = M(ModT::CrushBits); c.crush.rate = M(ModT::CrushRate); c.crush.mix = pCrushMix->load();
    c.lowCutHz = M(ModT::LowCut); c.highCutHz = M(ModT::HighCut);
    c.lowSlope = int(pLowSlope->load()); c.highSlope = int(pHighSlope->load()); c.tilt = M(ModT::Tilt);
    engine.setParams(c);
}

void CoreAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    hostBpm = zwui::readTransport(*this, motion, hostBpm);
    meterIn.push(buffer, motion.levelIn);
    spectrumIn.push(buffer);
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    power.begin(buffer, pPower->load() > 0.5f);
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
    for (int b = 0; b < 3; ++b) bandGainDb[size_t(b)].store(engine.bandAutoGainDb(b));
    power.end(buffer);
    meterOut.push(buffer, motion.levelOut);
    if (wetScratch.getNumSamples() >= n && numOut >= 2) { juce::AudioBuffer<float> wet(wetScratch.getArrayOfWritePointers(), 2, n); spectrum.push(wet); }
    else spectrum.push(buffer);
    stereo.push(buffer);
}

void CoreAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= Presets::count()) return;
    currentPreset = index;
    zwui::resetModParams(apvts);
    Presets::apply(index, apvts);
    bank.clearCurrentUser();
}

void CoreAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    bank.writeTo(state);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void CoreAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
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

juce::AudioProcessorEditor* CoreAudioProcessor::createEditor() { return new CoreAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CoreAudioProcessor(); }
