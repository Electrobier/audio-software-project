// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
class IronLook final : public juce::LookAndFeel_V4
{
  public:
    IronLook();
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float, juce::Slider &) override;
};
class IronEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit IronEditor(IronProcessor &);
    ~IronEditor() override;
    void resized() override;

  private:
    struct Face final : juce::Component
    {
        IronEditor &owner;
        explicit Face(IronEditor &e) : owner(e) {}
        void paint(juce::Graphics &) override;
    };
    IronProcessor &processor;
    IronLook look;
    Face face;
    juce::TooltipWindow tooltips{this, 600};
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> choices;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> choiceAttachments;
    juce::ToggleButton analog{"ANALOG"}, bypass{"BYPASS"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analogAttachment, bypassAttachment;
    juce::ComboBox preset;
    juce::TextButton previous{"<"}, next{">"}, ab{"A / B"}, menu{"OPTIONS"};
    std::unique_ptr<juce::FileChooser> chooser;
    std::array<float, 2> needles{3, 3};
    void knob(const char *, const char *, juce::Rectangle<int>, bool red = false);
    void choice(const char *, const char *, juce::Rectangle<int>);
    void label(const juce::String &, juce::Rectangle<int>);
    void timerCallback() override;
    void showMenu();
    void drawMeter(juce::Graphics &, juce::Rectangle<float>, float, int);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IronEditor)
};
