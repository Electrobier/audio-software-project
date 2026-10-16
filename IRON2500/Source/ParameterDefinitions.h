// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
namespace iron
{
inline juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto f = [&](const char *id, const char *name, float lo, float hi, float def, float step = 0.01f)
    {
        l.add(std::make_unique<AudioParameterFloat>(ParameterID{id, 1}, name,
                                                    NormalisableRange<float>{lo, hi, step}, def));
    };
    auto c = [&](const char *id, const char *name, StringArray choices, int def)
    { l.add(std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, name, choices, def)); };
    f("threshold", "Threshold (dB)", -20, 10, -10);
    c("attack", "Attack (ms)", {"0.03", "0.1", "0.3", "1", "3", "10", "30"}, 4);
    c("ratio", "Ratio", {"1.5:1", "2:1", "3:1", "4:1", "6:1", "10:1", "Inf:1"}, 1);
    c("release", "Release (s)", {"0.05", "0.1", "0.2", "0.5", "1", "2"}, 2);
    f("variable", "Variable Release (%)", 0, 100, 0);
    c("knee", "Knee", {"Hard", "Medium", "Soft"}, 1);
    c("thrust", "Thrust", {"Normal", "Medium", "Loud"}, 0);
    c("type", "Type", {"New / FF", "Old / FB", "F Back / RMS"}, 0);
    c("shape", "Link Shape", {"BP", "HP", "LP", "IND"}, 3);
    f("link", "L/R Link (%)", 0, 100, 100);
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"analog", 1}, "Analog", false));
    f("output", "Output (dB)", -24, 24, 0);
    f("mix", "Mix (%)", 0, 100, 100);
    f("trim", "Input Trim (dB)", -18, 18, 0);
    c("meter", "VU Source", {"GR", "Output", "Input"}, 0);
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"bypass", 1}, "Bypass", false));
    return l;
}
inline constexpr std::array<const char *, 16> parameterIds{
    "threshold", "attack", "ratio",  "release", "variable", "knee", "thrust", "type",
    "shape",     "link",   "analog", "output",  "mix",      "trim", "meter",  "bypass"};
struct Preset
{
    const char *name;
    std::array<float, 16> values;
};
inline constexpr std::array<Preset, 12> presets{
    {{"Default / Clean", {-10, 4, 1, 2, 0, 1, 0, 0, 3, 100, 0, 0, 100, 0, 0, 0}},
     {"Mix Bus Glue", {-12, 5, 1, 3, 35, 2, 1, 1, 3, 100, 1, 2, 100, 0, 0, 0}},
     {"Rock Drum Punch", {-14, 6, 3, 1, 10, 0, 2, 0, 1, 85, 1, 4, 100, 0, 0, 0}},
     {"Aggressive Drum Bus", {-18, 3, 5, 0, 15, 0, 2, 0, 0, 90, 1, 8, 100, 0, 0, 0}},
     {"Parallel Drum Smash", {-20, 2, 6, 1, 40, 0, 1, 0, 3, 100, 1, 9, 35, 0, 0, 0}},
     {"Electronic Kick Control", {-12, 2, 3, 1, 0, 1, 0, 0, 3, 100, 0, 3, 100, 0, 0, 0}},
     {"Bass Tightening", {-16, 4, 2, 2, 50, 2, 0, 2, 2, 90, 1, 3, 100, 0, 0, 0}},
     {"Vocal Bus", {-14, 4, 1, 3, 60, 2, 1, 1, 3, 100, 0, 2, 100, 0, 0, 0}},
     {"Gentle Master Bus", {-8, 6, 0, 4, 65, 2, 1, 1, 3, 100, 0, 1, 100, 0, 0, 0}},
     {"Analog Warmth", {5, 5, 0, 3, 25, 2, 0, 1, 3, 100, 1, -3, 100, 3, 0, 0}},
     {"Fast Transient Control", {-10, 0, 5, 0, 0, 0, 0, 0, 3, 100, 0, 3, 100, 0, 0, 0}},
     {"Slow Musical Compression", {-14, 6, 1, 4, 85, 2, 1, 2, 3, 100, 1, 2, 100, 0, 0, 0}}}};
} // namespace iron
