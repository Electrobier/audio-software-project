#pragma once
#include "PluginProcessor.h"
class PanelLook final : public juce::LookAndFeel_V4 {
public:
  bool realistic = true;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
  void drawButtonText(juce::Graphics &, juce::TextButton &, bool,
                      bool) override;
  void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider::SliderStyle,
                        juce::Slider &) override;
};
class FineFader final : public juce::Slider {
  void mouseDown(const juce::MouseEvent &e) override {
    setMouseDragSensitivity(
        (e.mods.isShiftDown() || e.mods.isCtrlDown() || e.mods.isCommandDown())
            ? 2400
            : 300);
    juce::Slider::mouseDown(e);
  }
};
class White224Editor final : public juce::AudioProcessorEditor,
                             private juce::Timer {
public:
  explicit White224Editor(White224Processor &);
  ~White224Editor() override;
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  void timerCallback() override;
  void bindFaders();
  void chooseFile(bool save);
  void refreshPresets();
  void persist();
  void drawDisplay(juce::Graphics &);
  void segmentText(juce::Graphics &, juce::String, float, float, float);
  White224Processor &processor;
  PanelLook look;
  juce::Component panel;
  std::array<juce::TextButton, 8> programs, functions;
  std::array<juce::TextButton, 6> values;
  std::array<FineFader, 6> faders;
  std::array<
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 6>
      attachments;
  juce::TextButton wet{"100% WET"}, bypass{"BYPASS"}, design{"3D LOOK"},
      save{"SAVE"}, load{"LOAD"};
  juce::ComboBox presets;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      wetAttach, bypassAttach;
  std::unique_ptr<juce::FileChooser> chooser;
  juce::Array<juce::File> userFiles;
  bool shift = false, call = true, armed = false;
  int held = -1, lastFactory = -1;
  float levelL = 0, levelR = 0;
  int regFlash = -1, flashTicks = 0;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(White224Editor)
};
