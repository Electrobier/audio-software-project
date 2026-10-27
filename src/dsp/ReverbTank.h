#pragma once
#include "Primitives.h"
#include "presets/FactoryPresets.h"
namespace white224 {
class ReverbTank {
  std::array<Delay, 8> lines;
  std::array<float, 8> lengths{}, low{}, high{}, phase{}, bassGain{}, midGain{},
      highGain{}, targetBass{}, targetMid{}, targetHigh{};
  double sampleRate = 48000;
  float lowAlpha = .06f, highAlpha = .5f, modAmount = 0, modTarget = 0;
  bool firstUpdate = true;
  int rotation = 0;
  float modScale = 1;

public:
  void prepare(double sr) {
    sampleRate = sr;
    for (auto &l : lines)
      l.prepare(static_cast<size_t>(sr * .13));
  }
  void reset(const Program &p) noexcept {
    for (int i = 0; i < 8; ++i) {
      lines[i].reset();
      int length = static_cast<int>(sampleRate * p.delaysMs[i] * .001);
      auto prime = [](int n) {
        if (n < 2)
          return false;
        for (int k = 2; k * k <= n; ++k)
          if (n % k == 0)
            return false;
        return true;
      };
      while (!prime(length))
        ++length;
      lengths[i] = static_cast<float>(length);
      low[i] = high[i] = 0;
      phase[i] = static_cast<float>(i * .781);
    }
    rotation = p.rotation;
    modScale = p.modulation;
    firstUpdate = true;
    modAmount = modTarget = 0;
  }
  void update(const Settings &s, float activity) noexcept {
    lowAlpha = static_cast<float>(
        1 - std::exp(-2 * pi *
                     std::min(s.values[Crossover],
                              static_cast<float>(sampleRate * .4)) /
                     sampleRate));
    highAlpha = static_cast<float>(
        1 - std::exp(-2 * pi *
                     std::min(s.values[Treble],
                              static_cast<float>(sampleRate * .4)) /
                     sampleRate));
    const float reduction =
        s.decayEnabled ? 1.f / (1.f + activity * s.values[Optimization] * .035f)
                       : 1.f;
    modTarget = s.modeEnabled ? s.values[Enhancement] * .000018f *
                                    static_cast<float>(sampleRate) * modScale
                              : 0.f;
    for (int i = 0; i < 8; ++i) {
      float seconds = lengths[i] / static_cast<float>(sampleRate);
      targetBass[i] =
          std::pow(10.f, -3 * seconds / (s.values[Bass] * reduction));
      targetMid[i] = std::pow(10.f, -3 * seconds / (s.values[Mid] * reduction));
      targetHigh[i] = std::pow(
          10.f, -3 * seconds /
                    (s.values[Mid] * reduction *
                     (.27f + .35f * (1 - programs()[s.program].damping))));
      if (firstUpdate) {
        bassGain[i] = targetBass[i];
        midGain[i] = targetMid[i];
        highGain[i] = targetHigh[i];
      }
    }
    if (firstUpdate) {
      modAmount = modTarget;
      firstUpdate = false;
    }
  }
  std::array<float, 2> process(float left, float right) noexcept {
    std::array<float, 8> v{}, raw{};
    modAmount += .001f * (modTarget - modAmount);
    for (int i = 0; i < 8; ++i) {
      bassGain[i] += .001f * (targetBass[i] - bassGain[i]);
      midGain[i] += .001f * (targetMid[i] - midGain[i]);
      highGain[i] += .001f * (targetHigh[i] - highGain[i]);
      phase[i] += static_cast<float>((.031 + i * .007) * 2 * pi / sampleRate);
      if (phase[i] > 2 * pi)
        phase[i] -= static_cast<float>(2 * pi);
      raw[i] = lines[i].read(lengths[i] + modAmount * std::sin(phase[i]));
      low[i] = clean(low[i] + lowAlpha * (raw[i] - low[i]));
      high[i] = clean(high[i] + highAlpha * (raw[i] - high[i]));
      v[i] = low[i] * bassGain[i] + (high[i] - low[i]) * midGain[i] +
             (raw[i] - high[i]) * highGain[i];
    }
    for (int step = 1; step < 8; step *= 2)
      for (int j = 0; j < 8; j += 2 * step)
        for (int k = 0; k < step; ++k) {
          float a = v[j + k], b = v[j + k + step];
          v[j + k] = a + b;
          v[j + k + step] = a - b;
        }
    for (int i = 0; i < 8; ++i) {
      const float inject = ((i & 1) ? right : left) * ((i & 2) ? -.23f : .23f);
      lines[i].push(v[i] * .3535533906f + inject);
    }
    // Distinct orthogonal sign vectors; rotate tap ordering per program.
    constexpr float sl[8] = {1, 1, -1, 1, -1, -1, 1, -1},
                    sr[8] = {1, -1, 1, 1, -1, 1, -1, -1};
    float l = 0, r = 0;
    for (int i = 0; i < 8; ++i) {
      float x = raw[(i + rotation) % 8];
      l += sl[i] * x;
      r += sr[i] * x;
    }
    return {l * .35355339f, r * .35355339f};
  }
};
} // namespace white224
