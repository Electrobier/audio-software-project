#pragma once
#include "dsp/ReverbEngine.h"
#include <JuceHeader.h>
class White224Processor final : public juce::AudioProcessor {
public:
  White224Processor();
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  void processBlockBypassed(juce::AudioBuffer<float> &,
                            juce::MidiBuffer &) override;
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override {
    return "White224 Vintage Reverb";
  }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 80; }
  int getNumPrograms() override { return 42; }
  int getCurrentProgram() override { return presetIndex.load(); }
  void setCurrentProgram(int i) override;
  const juce::String getProgramName(int i) override;
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  juce::AudioProcessorParameter *getBypassParameter() const override;
  white224::Settings settings() const;
  void apply(const white224::Settings &);
  void selectAlgorithm(int, bool loadRecommended);
  void storeRegister(int);
  bool recallRegister(int);
  bool hasRegister(int) const;
  juce::Result savePreset(const juce::File &);
  juce::Result loadPreset(const juce::File &);
  juce::File userPresetDirectory() const;
  void setParameter(const juce::String &, float);
  void setUi(int width, int height, bool realistic, bool shift, bool call);
  std::array<int, 2> uiSize() const;
  bool uiFlag(const juce::String &, bool) const;
  juce::AudioProcessorValueTreeState state;
  std::atomic<float> meterL{0}, meterR{0};

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  juce::ValueTree snapshot();
  bool restore(const juce::ValueTree &);
  static bool validParameters(const juce::ValueTree &);
  void render(juce::AudioBuffer<float> &, bool hostBypass);
  white224::ReverbEngine engine;
  std::array<std::atomic<float> *, white224::Count> raw{};
  std::atomic<float> *algorithm = nullptr, *mode = nullptr, *decay = nullptr,
                     *wet = nullptr, *bypass = nullptr;
  std::array<juce::ValueTree, 4> registers;
  juce::ValueTree ui{"UI"};
  mutable juce::CriticalSection metadataLock;
  std::atomic<int> presetIndex{0};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(White224Processor)
};
