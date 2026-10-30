#include <JuceHeader.h>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    proc->prepareToPlay(48000.0, 512);
    if (argc > 2) proc->setCurrentProgram(atoi(argv[2]));
    // Run some stereo audio through so the visualisers have content
    {
        juce::AudioBuffer<float> b(2, 512); juce::MidiBuffer mb; double ph = 0;
        for (int blk = 0; blk < 60; ++blk)
        {
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
