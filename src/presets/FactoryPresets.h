#pragma once
#include "Parameters.h"
#include <array>
#include <string>
namespace white224 {
struct Program {
  const char *name;
  std::array<float, 8> delaysMs;
  int diffusion;
  float diffusionGain, earlyGain, modulation, damping;
  int rotation;
};
const std::array<Program, 9> &programs();
Settings recommended(int program);
struct Preset {
  std::string bank, name;
  Settings settings;
  bool reconstructed;
};
const std::array<Preset, 42> &factoryPresets();
} // namespace white224
