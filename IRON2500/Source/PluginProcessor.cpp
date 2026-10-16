// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#include "PluginProcessor.h"
#include "PluginEditor.h"
IronProcessor::IronProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, &undo, "IRON2500", iron::makeLayout())
{
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = state.getRawParameterValue(iron::parameterIds[i]);
    alternate = state.copyState();
    for (size_t i = 2; i < meters.size(); ++i)
        meters[i].store(-120);
}
bool IronProcessor::isBusesLayoutSupported(const BusesLayout &l) const
{
    auto c = l.getMainOutputChannelSet();
    return (c == juce::AudioChannelSet::mono() || c == juce::AudioChannelSet::stereo()) &&
           c == l.getMainInputChannelSet();
}
void IronProcessor::prepareToPlay(double r, int)
{
    engine.prepare(r);
    setLatencySamples(iron::CompressorEngine::latency);
}
iron::Parameters IronProcessor::readParameters(bool bypassed)
{
    std::array<float, 16> v{};
    for (size_t i = 0; i < v.size(); ++i)
        v[i] = values[i]->load(std::memory_order_relaxed);
    constexpr double attacks[]{.03, .1, .3, 1, 3, 10, 30}, ratios[]{1.5, 2, 3, 4, 6, 10, 1000},
        releases[]{.05, .1, .2, .5, 1, 2}, knees[]{0, 6, 12};
    iron::Parameters p;
    p.threshold = v[0];
    p.attack = attacks[juce::jlimit(0, 6, (int)v[1])];
    p.ratio = ratios[juce::jlimit(0, 6, (int)v[2])];
    p.release = releases[juce::jlimit(0, 5, (int)v[3])];
    p.variable = v[4] * .01;
    p.knee = knees[juce::jlimit(0, 2, (int)v[5])];
    p.thrust = (int)v[6];
    p.type = (int)v[7];
    p.shape = (int)v[8];
    p.link = v[9] * .01;
    p.analog = v[10];
    p.output = v[11];
    p.mix = v[12] * .01;
    p.trim = v[13];
    p.bypass = bypassed ? 1 : v[15];
    return p;
}
template <typename T> void IronProcessor::process(juce::AudioBuffer<T> &b, bool bypassed)
{
    juce::ScopedNoDenormals noDenormals;
    engine.setParameters(readParameters(bypassed));
    auto *l = b.getWritePointer(0);
    auto *r = b.getNumChannels() > 1 ? b.getWritePointer(1) : nullptr;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        double x = l[i], y = r ? r[i] : x;
        engine.process(x, y, r ? 2 : 1);
        l[i] = (T)x;
        if (r)
            r[i] = (T)y;
    }
    for (int mode = 0; mode < 3; ++mode)
        for (int ch = 0; ch < 2; ++ch)
            meters[(size_t)(mode * 2 + ch)].store((float)engine.meter(ch, mode), std::memory_order_relaxed);
}
void IronProcessor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &)
{
    process(b, false);
}
void IronProcessor::processBlock(juce::AudioBuffer<double> &b, juce::MidiBuffer &)
{
    process(b, false);
}
void IronProcessor::processBlockBypassed(juce::AudioBuffer<float> &b, juce::MidiBuffer &)
{
    process(b, true);
}
void IronProcessor::processBlockBypassed(juce::AudioBuffer<double> &b, juce::MidiBuffer &)
{
    process(b, true);
}
void IronProcessor::setCurrentProgram(int i)
{
    program = juce::jlimit(0, 11, i);
    undo.beginNewTransaction("Factory preset");
    for (size_t n = 0; n < values.size(); ++n)
    {
        auto *p = state.getParameter(iron::parameterIds[n]);
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(iron::presets[(size_t)program.load()].values[n]));
        p->endChangeGesture();
    }
}
void IronProcessor::getStateInformation(juce::MemoryBlock &dest)
{
    auto tree = state.copyState();
    tree.setProperty("program", program.load(), nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary(*xml, dest);
}
void IronProcessor::setStateInformation(const void *d, int s)
{
    if (auto xml = getXmlFromBinary(d, s))
    {
        if (xml->hasTagName(state.state.getType()))
        {
            auto t = juce::ValueTree::fromXml(*xml);
            program = juce::jlimit(0, 11, (int)t.getProperty("program", 0));
            state.replaceState(t);
        }
    }
}
void IronProcessor::swapAB()
{
    auto t = state.copyState();
    state.replaceState(alternate);
    alternate = t;
}
bool IronProcessor::savePreset(const juce::File &f)
{
    juce::MemoryBlock b;
    getStateInformation(b);
    return f.replaceWithData(b.getData(), b.getSize());
}
bool IronProcessor::loadPreset(const juce::File &f)
{
    juce::MemoryBlock b;
    if (!f.loadFileAsData(b))
        return false;
    auto xml = getXmlFromBinary(b.getData(), (int)b.getSize());
    if (!xml || !xml->hasTagName(state.state.getType()))
        return false;
    setStateInformation(b.getData(), (int)b.getSize());
    return true;
}
juce::AudioProcessorEditor *IronProcessor::createEditor()
{
    return new IronEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new IronProcessor();
}
