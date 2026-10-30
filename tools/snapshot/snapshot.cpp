#include <JuceHeader.h>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    proc->prepareToPlay(48000.0, 512);
    if (argc > 2) proc->setCurrentProgram(atoi(argv[2]));
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
