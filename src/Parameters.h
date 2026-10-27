#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace white224 {
enum Param {
  Bass,
  Mid,
  Crossover,
  Treble,
  Depth,
  Predelay,
  Mix,
  Input,
  Output,
  Bandwidth,
  Enhancement,
  Optimization,
  Count
};
struct ParameterSpec {
  const char *id;
  const char *name;
  const char *unit;
  float min, max, initial;
  bool logarithmic;
};
inline constexpr std::array<ParameterSpec, Count> specs{
    {{"bass", "BASS", "sec", .6f, 70, 2.83f, true},
     {"mid", "MID", "sec", .6f, 70, 2.27f, true},
     {"crossover", "CROSS OVER", "Hz", 100, 10900, 433, true},
     {"treble", "TREBLE DECAY", "Hz", 100, 10900, 4300, true},
     {"depth", "DEPTH", "", 0, 71, 35, false},
     {"predelay", "PRE DELAY", "ms", 0, 255, 4, false},
     {"mix", "MIX", "%", 0, 100, 35, false},
     {"input", "INPUT", "dB", -12, 12, 2, false},
     {"output", "OUTPUT", "dB", -60, 12, 4, false},
     {"bandwidth", "BANDWIDTH", "Hz", 3000, 20000, 8100, true},
     {"enhancement", "MODE ENHANCEMENT", "", 1, 16, 4, false},
     {"optimization", "DECAY OPTIMIZATION", "", 1, 16, 4, false}}};
struct Settings {
  std::array<float, Count> values{};
  int program = 0;
  bool modeEnabled = true, decayEnabled = true, wet = false, bypass = false;
  Settings() {
    for (int i = 0; i < Count; ++i)
      values[i] = specs[i].initial;
  }
  void sanitize() {
    program = std::clamp(program, 0, 8);
    for (int i = 0; i < Count; ++i)
      values[i] = std::isfinite(values[i])
                      ? std::clamp(values[i], specs[i].min, specs[i].max)
                      : specs[i].initial;
  }
};
} // namespace white224
