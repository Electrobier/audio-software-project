#pragma once
#include "ReverbTank.h"
#include "VintageConverter.h"
namespace white224 {
class ReverbCore {
  std::array<BandwidthFilter, 2> bandwidth;
  std::array<Delay, 2> early, chorus;
  std::array<CrossfadeDelay, 2> pre, body;
  std::array<std::array<Allpass, 6>, 2> diffusion;
  ReverbTank tank;
  double rate = 48000;
  int program = 0, counter = 0;
  float envelope = 0;
  float phase = 0, envAttack = .001f, envRelease = .0001f;
  Settings current;

public:
  void prepare(double sr);
  void reset(int p) noexcept;
  void update(const Settings &s) noexcept;
  std::array<float, 2> process(float l, float r) noexcept;
};
class ReverbEngine {
  std::array<ReverbCore, 2> cores;
  std::array<Smooth, Count> smooth;
  Smooth bypass, wet;
  Settings target, current;
  double sampleRate = 48000;
  int active = 0, incoming = 1, transition = 0, transitionLength = 3840;
  bool initialized = false;

public:
  void prepare(double sr);
  void reset();
  void setSettings(Settings s) noexcept;
  void process(float *left, float *right, int samples) noexcept;
  float peakLeft = 0, peakRight = 0;
};
} // namespace white224
