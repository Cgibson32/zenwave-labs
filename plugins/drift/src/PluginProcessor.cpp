#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;


DriftAudioProcessor::DriftAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
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
    pColor = apvts.getRawParameterValue(ParamIDs::color);
    mod.attach(apvts, driftModTargets());
    pad.attach(apvts);
    bank.setAlwaysLocked({ zwui::PadIDs::x, zwui::PadIDs::y, zwui::PadIDs::hold, zwui::PadIDs::glide, zwui::FreezeQuantizer::paramId });
    pFreezeQ = apvts.getRawParameterValue(zwui::FreezeQuantizer::paramId);
    bank.setAlwaysLocked({ ParamIDs::power, ParamIDs::output, ParamIDs::mix, ParamIDs::freeze, zwui::OrbitBus::orbitParamId });
    bank.setSafeRange(ParamIDs::feedback, 0.0f, 0.82f);
    bank.setSafeRange(ParamIDs::lowCut, 20.0f, 400.0f);
    bank.setSafeRange(ParamIDs::highCut, 2500.0f, 20000.0f);
    bank.setSafeRange(ParamIDs::width, 0.6f, 1.5f);
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
    params.push_back(std::make_unique<P>(juce::ParameterID{ ParamIDs::color, 1 }, "Color", Rng{ -1.0f, 1.0f, 0.001f }, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return v < -0.01f ? "Dark " + juce::String(juce::roundToInt(-v * 100)) + " %" : v > 0.01f ? "Thin " + juce::String(juce::roundToInt(v * 100)) + " %" : juce::String("Off"); })));
    zwui::addModParams(params, driftModTargets());
    zwui::addOrbitParam(params);
    zwui::addPadParams(params);
    zwui::FreezeQuantizer::addParam(params);
    return { params.begin(), params.end() };
}

void DriftAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare(sampleRate * 2.0, samplesPerBlock * 2);
    engine.setInternalOversampling(2);
    osBuf.setSize(2, samplesPerBlock * 2); osWet.setSize(2, samplesPerBlock * 2);
    for (int c = 0; c < 2; ++c) { osUp[c].reset(); osDown[c].reset(); osWetDown[c].reset(); }
    setLatencySamples(3);
    mod.prepare(sampleRate);
    pad.prepare(sampleRate);
    wetScratch.setSize(2, samplesPerBlock);
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
    if (! (in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo()))) return false;
    const auto& sc = layouts.getChannelSet(true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

float DriftAudioProcessor::syncedTimeMs() const
{
    return float(zwui::divisionSeconds(int(pDivision->load()), hostBpm) * 1000.0);
}

float DriftAudioProcessor::padTimeMs(float x) const
{
    if (padSynced()) return float(zwui::divisionSeconds(juce::roundToInt(x * 9.0f), hostBpm) * 1000.0);   // 1/32 .. 1/2, snapped
    return 40.0f * std::pow(1500.0f / 40.0f, x);
}
float DriftAudioProcessor::padXForTime(float ms) const
{
    if (padSynced()) return juce::jlimit(0.0f, 1.0f, float(juce::jlimit(0, 9, int(pDivision->load()))) / 9.0f);
    return juce::jlimit(0.0f, 1.0f, std::log(ms / 40.0f) / std::log(1500.0f / 40.0f));
}

void DriftAudioProcessor::updateEngineParams()
{
    zw::DriftParams p;
    const auto M = [this](int t) { return mod.value(t); };
    float timeMs = pSync->load() > 0.5f ? syncedTimeMs() * std::pow(2.0f, mod.offsetNorm(ModT::Time) * 2.0f) : M(ModT::Time);   // synced: +/-2 octaves
    float fb     = M(ModT::Feedback);
    // Perform pad override, eased in/out by PadEngine (time interpolates in the log domain)
    const float pa = pad.eased.load();
    if (pa > 0.0001f)
    {
        timeMs = std::exp(juce::jmap(pa, std::log(timeMs), std::log(padTimeMs(pad.x()))));
        fb     = juce::jmap(pa, fb, padFeedback(pad.y()));
    }
    pad.effX.store(padXForTime(timeMs)); pad.effY.store(padYForFeedback(fb));
    p.timeMs     = timeMs;
    p.feedback   = fb;
    p.wowFlutter = M(ModT::Wobble);
    p.age        = M(ModT::Age);
    p.space      = M(ModT::Space);
    p.size       = M(ModT::Size);
    p.decay      = M(ModT::Decay);
    p.width      = M(ModT::Width);
    p.spread     = M(ModT::Spread);
    p.pingPong   = pPingPong->load() > 0.5f;
    p.freeze     = freezeQ.effective;
    p.mix        = M(ModT::Mix);
    p.output     = pOutput->load();
    p.wobbleHz   = pWobbleSync->load() > 0.5f
                     ? float(1.0 / zwui::divisionSeconds(int(pWobbleDiv->load()), hostBpm))
                     : M(ModT::WobbleRate);
    p.lowCutHz  = M(ModT::LowCut);  p.highCutHz = M(ModT::HighCut);
    p.lowSlope  = int(pLowSlope->load()); p.highSlope = int(pHighSlope->load());
    p.tilt      = M(ModT::Tilt);
    p.color     = M(ModT::Color);
    engine.setParams(p);
}

void DriftAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    hostBpm = zwui::readTransport(*this, motion, hostBpm);
    const float* scL = nullptr; const float* scR = nullptr;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled())
    {
        auto sc = getBusBuffer(buffer, true, 1);
        if (sc.getNumChannels() > 0) { scL = sc.getReadPointer(0); scR = sc.getNumChannels() > 1 ? sc.getReadPointer(1) : scL; }
    }
    meterIn.push(buffer, motion.levelIn);
    spectrumIn.push(buffer);

    const int n = buffer.getNumSamples();
    const int numIn = getMainBusNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    power.begin(buffer, pPower->load() > 0.5f);
    if (power.skipProcessing()) { mod.process(buffer.getReadPointer(0), numIn > 1 ? buffer.getReadPointer(1) : nullptr, n, hostBpm, motion.ppq.load(), motion.playing.load(), scL, scR); updateEngineParams(); motion.levelOut.store(0.0f); return; }

    if (numOut >= 2 && numIn == 1) buffer.copyFrom(1, 0, buffer, 0, 0, n);

    // Sub-block loop so modulation moves smoothly inside large host buffers
    constexpr int kSub = 64;
    const bool playing = motion.playing.load();
    double ppq = motion.ppq.load();
    for (int start = 0; start < n; start += kSub)
    {
        const int len = juce::jmin(kSub, n - start);
        mod.process(buffer.getReadPointer(0, start), numOut >= 2 ? buffer.getReadPointer(1, start) : nullptr, len, hostBpm, ppq, playing, scL ? scL + start : nullptr, scR ? scR + start : nullptr);
        pad.process(len, hostBpm);
        {
            const double ppqEnd = playing ? ppq + len / currentSampleRate * hostBpm / 60.0 : ppq;
            const bool want = pFreeze->load() > 0.5f;
            freezeQ.advance(want, int(pFreezeQ->load()), playing, ppq, ppqEnd);
        }
        updateEngineParams();
        // --- 2x oversampled engine pass ---
        {
            const int chans = numOut >= 2 ? 2 : 1;
            float* up[2] = { osBuf.getWritePointer(0), osBuf.getWritePointer(1) };
            for (int c = 0; c < chans; ++c)
            {
                const float* src = buffer.getReadPointer(c, start);
                for (int i = 0; i < len; ++i) osUp[c].upsample(src[i], up[c][2 * i], up[c][2 * i + 1]);
            }
            const bool tap = wetScratch.getNumSamples() >= n && chans == 2;
            if (tap) engine.setWetTap(osWet.getWritePointer(0), osWet.getWritePointer(1));
            if (chans == 2) engine.process(up[0], up[1], len * 2);
            else engine.processMono(up[0], len * 2);
            engine.setWetTap(nullptr, nullptr);
            for (int c = 0; c < chans; ++c)
            {
                float* dst = buffer.getWritePointer(c, start);
                for (int i = 0; i < len; ++i) dst[i] = osDown[c].downsample(up[c][2 * i], up[c][2 * i + 1]);
                if (tap) { const float* w = osWet.getReadPointer(c); float* wd = wetScratch.getWritePointer(c, start); for (int i = 0; i < len; ++i) wd[i] = osWetDown[c].downsample(w[2 * i], w[2 * i + 1]); }
            }
        }
        if (playing) ppq += len / currentSampleRate * hostBpm / 60.0;
    }
    for (int ch = 2; ch < numOut; ++ch) buffer.clear(ch, 0, n);

    power.end(buffer);
    meterOut.push(buffer, motion.levelOut);
    if (wetScratch.getNumSamples() >= n && numOut >= 2) { juce::AudioBuffer<float> wet(wetScratch.getArrayOfWritePointers(), 2, n); spectrum.push(wet); }
    else spectrum.push(buffer);
    stereo.push(buffer);
}

void DriftAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= Presets::count()) return;
    currentPreset = index;
    zwui::resetModParams(apvts);
    Presets::apply(index, apvts);
    bank.clearCurrentUser();
}

void DriftAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    bank.writeTo(state);
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
            bank.readFrom(tree);
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
