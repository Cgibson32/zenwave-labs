#include <JuceHeader.h>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    proc->prepareToPlay(48000.0, 512);
    if (argc > 2) proc->setCurrentProgram(atoi(argv[2]));
    // "wav:<path>" as the 4th argument: render an audition of the preset (synths: a held chord + release) and exit
    if (argc > 3 && juce::String(argv[3]).startsWith("wav:"))
    {
        const double sr = 48000.0; const int blockN = 512; const int holdBlocks = int(sr * 10.0 / blockN), tailBlocks = int(sr * 6.0 / blockN);
        juce::AudioBuffer<float> out(2, (holdBlocks + tailBlocks) * blockN); out.clear();
        juce::AudioBuffer<float> b(2, blockN); juce::MidiBuffer mb;
        const bool synth = proc->acceptsMidi();
        if (synth) { for (int n : { 48, 55, 62, 64, 67 }) mb.addEvent(juce::MidiMessage::noteOn(1, n, 0.7f), 0); }
        for (int blk = 0; blk < holdBlocks + tailBlocks; ++blk)
        {
            b.clear();
            if (! synth) for (int i = 0; i < blockN; ++i) { const double t = (blk * blockN + i) / sr; const float v = 0.4f * std::sin(float(2 * M_PI * 110.0 * t)) * float(std::exp(-std::fmod(t, 0.5) * 5.0)); b.setSample(0, i, v); b.setSample(1, i, v); }
            if (synth && blk == holdBlocks) for (int n : { 48, 55, 62, 64, 67 }) mb.addEvent(juce::MidiMessage::noteOff(1, n), 0);
            proc->processBlock(b, mb); mb.clear();
            for (int c = 0; c < 2; ++c) out.copyFrom(c, blk * blockN, b, c, 0, blockN);
        }
        juce::File wf(juce::String(argv[3]).fromFirstOccurrenceOf("wav:", false, false)); wf.deleteFile();
        std::unique_ptr<juce::AudioFormatWriter> w(juce::WavAudioFormat().createWriterFor(new juce::FileOutputStream(wf), sr, 2, 24, {}, 0));
        if (w) { w->writeFromAudioSampleBuffer(out, 0, out.getNumSamples()); w.reset(); std::printf("wrote %s\n", wf.getFullPathName().toRawUTF8()); }
        return 0;
    }
    // Run some stereo audio through so the visualisers have content
    {
        juce::AudioBuffer<float> b(2, 512); juce::MidiBuffer mb; double ph = 0;
        if (proc->acceptsMidi()) { mb.addEvent(juce::MidiMessage::noteOn(1, 48, 0.8f), 0); mb.addEvent(juce::MidiMessage::noteOn(1, 55, 0.7f), 0); mb.addEvent(juce::MidiMessage::noteOn(1, 64, 0.7f), 0); }
        for (int blk = 0; blk < 60; ++blk)
        {
            if (proc->acceptsMidi()) { b.clear(); proc->processBlock(b, mb); mb.clear(); continue; }
            for (int i = 0; i < 512; ++i)
            {
                const double t = (blk * 512 + i) / 48000.0;
                const float env = float(std::exp(-std::fmod(t, 0.43) * 6.0));
                const float bass = std::sin(float(2 * M_PI * 55.0 * t)) * env * 0.6f;
                const float pad  = 0.25f * (std::sin(float(2 * M_PI * 440.0 * t)) + std::sin(float(2 * M_PI * 554.4 * t + 0.4)));
                b.setSample(0, i, bass + pad * 0.9f + 0.1f * std::sin(float(2 * M_PI * 3300.0 * t)));
                b.setSample(1, i, bass + pad * 0.6f * std::cos(float(2 * M_PI * 0.7 * t)) + 0.1f * std::sin(float(2 * M_PI * 2900.0 * t + 1.0)));
            }
            proc->processBlock(b, mb);
        }
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc->createEditor());
    ed->setVisible(true);
    for (int i = 0; i < 30; ++i) { juce::MessageManager::getInstance()->runDispatchLoopUntil(10); }
    if (argc > 3)   // name of a button to click before the shot: "MANUAL", "FX", ...
    {
        const juce::String want = juce::String(argv[3]).toUpperCase();
        for (int i = 0; i < ed->getNumChildComponents(); ++i)
            if (auto* tb = dynamic_cast<juce::TextButton*>(ed->getChildComponent(i))) if (tb->getButtonText().toUpperCase() == want) { tb->triggerClick(); break; }
    }
    for (int i = 0; i < 10; ++i) { juce::MessageManager::getInstance()->runDispatchLoopUntil(10); }
    const float scale = 2.0f;
    auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, scale);
    juce::File out(argc > 1 ? argv[1] : "snapshot.png");
    out.deleteFile();
    juce::FileOutputStream os(out);
    juce::PNGImageFormat().writeImageToStream(img, os);
    std::printf("wrote %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset(); proc.reset();
    return 0;
}
