#include "ReverbEngine.h"
namespace white224 {
void ReverbCore::prepare(double sr) {
  rate = sr;
  envAttack = static_cast<float>(1 - std::exp(-1 / (rate * .015)));
  envRelease = static_cast<float>(1 - std::exp(-1 / (rate * .35)));
  tank.prepare(sr);
  for (int c = 0; c < 2; ++c) {
    pre[c].prepare(sr, .3);
    early[c].prepare(static_cast<size_t>(sr * .15));
    body[c].prepare(sr, .06);
    chorus[c].prepare(static_cast<size_t>(sr * .09));
    for (int i = 0; i < 6; ++i)
      diffusion[c][i].prepare(sr, 1.7f + i * 1.31f + c * .37f);
  }
  reset(0);
}
void ReverbCore::reset(int p) noexcept {
  program = p;
  counter = 0;
  phase = 0;
  envelope = 0;
  tank.reset(programs()[p]);
  for (int c = 0; c < 2; ++c) {
    bandwidth[c].reset();
    pre[c].reset();
    early[c].reset();
    body[c].reset();
    chorus[c].reset();
    for (auto &a : diffusion[c])
      a.reset();
  }
}
void ReverbCore::update(const Settings &s) noexcept {
  current = s;
  current.program = program;
  if (counter == 0) {
    for (auto &b : bandwidth)
      b.set(rate, s.values[Bandwidth]);
    tank.update(current, envelope);
  }
}
std::array<float, 2> ReverbCore::process(float l, float r) noexcept {
  const float gain = std::pow(10.f, current.values[Input] / 20.f);
  std::array<float, 2> x = {l * gain, r * gain};
  const float level = std::max(std::abs(x[0]), std::abs(x[1]));
  const float envCoef = level > envelope ? envAttack : envRelease;
  envelope += envCoef * (std::min(level * 3.f, 1.f) - envelope);
  if (++counter == 32)
    counter = 0;
  for (int c = 0; c < 2; ++c)
    x[c] = VintageConverter::process(bandwidth[c].process(x[c]));
  if (program == 8) {
    for (int c = 0; c < 2; ++c)
      chorus[c].push(x[c]);
    phase += static_cast<float>(2 * pi * .23 / rate);
    if (phase > 2 * pi)
      phase -= static_cast<float>(2 * pi);
    std::array<float, 2> y{};
    for (int v = 0; v < 4; ++v) {
      const float delay = static_cast<float>(rate) *
                          (.018f + v * .0031f +
                           current.values[Depth] / 71.f * .006f *
                               std::sin(phase * (1 + v * .13f) + v * 1.57f));
      const float a = chorus[v % 2].read(delay);
      y[0] += a * (v % 2 ? .16f : .34f);
      y[1] += a * (v % 2 ? .34f : .16f);
    }
    return y;
  }
  std::array<float, 2> reflections{};
  const auto &p = programs()[program];
  for (int c = 0; c < 2; ++c) {
    const float d = static_cast<float>(rate) * current.values[Predelay] * .001f;
    x[c] = pre[c].process(x[c], d);
    early[c].push(x[c]);
    for (int tap = 0; tap < 6; ++tap) {
      const float ms =
          (4.1f + tap * 7.13f + c * 1.17f) * (0.6f + program * .093f);
      reflections[c] += early[c].read(static_cast<float>(rate) * ms * .001f) *
                        ((tap % 2 ? -.12f : .19f) / (1 + tap * .18f));
    }
    for (int i = 0; i < p.diffusion; ++i)
      x[c] = diffusion[c][i].process(x[c], p.diffusionGain);
    const float depth =
        static_cast<float>(rate) * .045f * current.values[Depth] / 71.f;
    x[c] = body[c].process(x[c], depth);
  }
  const auto tail = tank.process(x[0], x[1]);
  return {tail[0] + reflections[0] * p.earlyGain,
          tail[1] + reflections[1] * p.earlyGain};
}
void ReverbEngine::prepare(double sr) {
  sampleRate = std::clamp(sr, 8000., 192000.);
  transitionLength = static_cast<int>(sampleRate * .08);
  for (auto &c : cores)
    c.prepare(sampleRate);
  for (auto &s : smooth)
    s.prepare(sampleRate);
  bypass.prepare(sampleRate, .01);
  wet.prepare(sampleRate, .01);
  initialized = false;
  reset();
}
void ReverbEngine::reset() {
  active = 0;
  incoming = 1;
  transition = 0;
  for (auto &c : cores)
    c.reset(target.program);
  current = target;
  for (int i = 0; i < Count; ++i)
    smooth[i].snap(target.values[i]);
  bypass.snap(target.bypass ? 1.f : 0.f);
  wet.snap(target.wet ? 1.f : 0.f);
  initialized = true;
}
void ReverbEngine::setSettings(Settings s) noexcept {
  s.sanitize();
  target = s;
  if (!initialized)
    return;
  for (int i = 0; i < Count; ++i)
    smooth[i].set(s.values[i]);
  bypass.set(s.bypass ? 1.f : 0.f);
  wet.set(s.wet ? 1.f : 0.f);
}
void ReverbEngine::process(float *left, float *right, int samples) noexcept {
  if (samples <= 0)
    return;
  // Reset inactive bank only at transition start. Storage is already allocated.
  if (transition == 0 && target.program != current.program) {
    incoming = 1 - active;
    cores[incoming].reset(target.program);
    current.program = target.program;
    transition = transitionLength;
  }
  peakLeft = peakRight = 0;
  for (int n = 0; n < samples; ++n) {
    for (int i = 0; i < Count; ++i)
      current.values[i] = smooth[i].next();
    current.modeEnabled = target.modeEnabled;
    current.decayEnabled = target.decayEnabled;
    cores[active].update(current);
    if (transition)
      cores[incoming].update(current);
    const float dl = std::isfinite(left[n]) ? left[n] : 0,
                dr = std::isfinite(right[n]) ? right[n] : 0;
    peakLeft = std::max(peakLeft, std::abs(dl));
    peakRight = std::max(peakRight, std::abs(dr));
    const float bp = bypass.next(), forceWet = wet.next();
    auto y = cores[active].process(dl * (1 - bp), dr * (1 - bp));
    if (transition) {
      const auto z = cores[incoming].process(dl * (1 - bp), dr * (1 - bp));
      const float f = 1.f - static_cast<float>(transition) / transitionLength;
      for (int c = 0; c < 2; ++c)
        y[c] += (z[c] - y[c]) * f;
      if (--transition == 0)
        active = incoming;
    }
    const float m = current.values[Mix] * .01f;
    const float mix = m + (1 - m) * forceWet;
    const float gain = std::pow(10.f, current.values[Output] / 20.f);
    left[n] = clean((dl * (1 - mix) + y[0] * mix) * gain * (1 - bp) + dl * bp);
    right[n] = clean((dr * (1 - mix) + y[1] * mix) * gain * (1 - bp) + dr * bp);
  }
}
} // namespace white224
