// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#pragma once
#include "DSP/CompressorEngine.h"
#include "ParameterDefinitions.h"
#include <juce_audio_processors/juce_audio_processors.h>
class IronProcessor final : public juce::AudioProcessor
{
  public:
    IronProcessor();
    const juce::String getName() const override
    {
        return "IRON 2500";
    }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlock(juce::AudioBuffer<double> &, juce::MidiBuffer &) override;
    void processBlockBypassed(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlockBypassed(juce::AudioBuffer<double> &, juce::MidiBuffer &) override;
    bool supportsDoublePrecisionProcessing() const override
    {
        return true;
    }
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override
    {
        return true;
    }
    bool acceptsMidi() const override
    {
        return false;
    }
    bool producesMidi() const override
    {
        return false;
    }
    double getTailLengthSeconds() const override
    {
        return 0;
    }
    int getNumPrograms() override
    {
        return (int)iron::presets.size();
    }
    int getCurrentProgram() override
    {
        return program;
    }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int i) override
    {
        return iron::presets[(size_t)juce::jlimit(0, 11, i)].name;
    }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::AudioProcessorParameter *getBypassParameter() const override
    {
        return state.getParameter("bypass");
    }
    void swapAB();
    bool savePreset(const juce::File &);
    bool loadPreset(const juce::File &);
    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState state;
    std::array<std::atomic<float>, 6> meters{};

  private:
    iron::CompressorEngine engine;
    std::array<std::atomic<float> *, 16> values{};
    juce::ValueTree alternate;
    std::atomic<int> program{0};
    iron::Parameters readParameters(bool);
    template <typename T> void process(juce::AudioBuffer<T> &, bool);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IronProcessor)
};
