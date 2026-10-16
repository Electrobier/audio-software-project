// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#include "PluginProcessor.h"
#include <iostream>
#include <juce_gui_extra/juce_gui_extra.h>
int main(int argc, char **argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    IronProcessor p;
    p.setCurrentProgram(4);
    juce::MemoryBlock data;
    p.getStateInformation(data);
    p.setCurrentProgram(2);
    p.setStateInformation(data.getData(), (int)data.getSize());
    if (std::abs(p.state.getRawParameterValue("mix")->load() - 35.0f) > .001f)
        return 1;
    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getNonexistentChildFile("iron-test", ".ironpreset", false);
    if (!p.savePreset(file))
        return 2;
    p.setCurrentProgram(8);
    if (!p.loadPreset(file))
        return 3;
    file.deleteFile();
    if (std::abs(p.state.getRawParameterValue("mix")->load() - 35.0f) > .001f)
        return 4;
    p.swapAB();
    p.swapAB();
    if (std::abs(p.state.getRawParameterValue("mix")->load() - 35.0f) > .001f)
        return 5;
    p.prepareToPlay(48000, 128);
    juce::AudioBuffer<float> b(2, 128);
    b.clear();
    juce::MidiBuffer midi;
    p.processBlock(b, midi);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 128; ++i)
            if (!std::isfinite(b.getSample(c, i)))
                return 6;
    for (double rate : {44100., 48000., 88200., 96000., 176400., 192000.})
    {
        for (int count : {1, 2})
        {
            p.setPlayConfigDetails(count, count, rate, 511);
            p.prepareToPlay(rate, 511);
            for (int block : {1, 7, 64, 511, 2048})
            {
                juce::AudioBuffer<double> doubles(count, block);
                for (int ch = 0; ch < count; ++ch)
                    for (int i = 0; i < block; ++i)
                        doubles.setSample(ch, i, .2 * std::sin(i * .1));
                p.processBlock(doubles, midi);
                for (int ch = 0; ch < count; ++ch)
                    for (int i = 0; i < block; ++i)
                        if (!std::isfinite(doubles.getSample(ch, i)))
                            return 11;
            }
            p.releaseResources();
        }
    }
    std::cout << "PASS processor Mono/Stereo, Double, six sample rates and five block sizes\n";
    if (argc == 3 && std::string(argv[1]) == "--preview")
    {
        p.setCurrentProgram(0);
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        editor->setSize(1200, 660);
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        juce::File previewFile(argv[2]);
        juce::FileOutputStream stream(previewFile);
        if (!juce::PNGImageFormat().writeImageToStream(image, stream))
            return 7;
        std::cout << "GUI snapshot written\n";
    }
    if (argc == 3 && std::string(argv[1]) == "--host")
    {
        juce::AudioPluginFormatManager manager;
        manager.addDefaultFormats();
        juce::OwnedArray<juce::PluginDescription> types;
        for (int i = 0; i < manager.getNumFormats(); ++i)
            manager.getFormat(i)->findAllTypesForFile(types, argv[2]);
        if (types.isEmpty())
            return 8;
        juce::String error;
        auto plugin = manager.createPluginInstance(*types[0], 48000, 128, error);
        if (!plugin)
        {
            std::cerr << error << "\n";
            return 9;
        }
        plugin->prepareToPlay(48000, 128);
        for (int n = 0; n < 1000; ++n)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 128; ++i)
                    b.setSample(ch, i, .25f * std::sin((n * 128 + i) * .1f));
            plugin->processBlock(b, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 128; ++i)
                    if (!std::isfinite(b.getSample(ch, i)))
                        return 10;
        }
        juce::MemoryBlock hostState;
        plugin->getStateInformation(hostState);
        plugin->setStateInformation(hostState.getData(), (int)hostState.getSize());
        plugin->releaseResources();
        std::cout << "PASS VST3 host discovery, instantiation, audio, state; latency="
                  << plugin->getLatencySamples() << "\n";
    }
    std::cout << "PASS state restore, disk preset, A/B and processor silence\n";
    return 0;
}
