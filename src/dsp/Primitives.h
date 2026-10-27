#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace white224 {
constexpr double pi = std::numbers::pi;
inline float clean(float x) noexcept {
  return std::isfinite(x) && std::abs(x) > 1e-25f ? x : 0.f;
}
class Delay {
  std::vector<float> data;
  size_t pos = 0;

public:
  void prepare(size_t n) {
    data.assign(n + 4, 0);
    pos = 0;
  }
  void reset() noexcept {
    std::fill(data.begin(), data.end(), 0);
    pos = 0;
  }
  void push(float x) noexcept {
    data[pos] = clean(x);
    if (++pos == data.size())
      pos = 0;
  }
  // pos is next write; delay=1 reads the last pushed sample.
  float read(float samples) const noexcept {
    double q = static_cast<double>(pos) -
               std::clamp(samples, 1.f, static_cast<float>(data.size() - 2));
    if (q < 0)
      q += data.size();
    const auto i = static_cast<size_t>(q);
    auto j = i + 1;
    if (j == data.size())
      j = 0;
    return data[i] + static_cast<float>(q - i) * (data[j] - data[i]);
  }
};
// Two stationary taps crossfade over 30 ms. Incoming automation is coalesced
// until the current crossfade ends, avoiding large read-head pitch jumps.
class CrossfadeDelay {
  Delay line;
  float a = 0, b = 0;
  int remaining = 0, length = 1440;
  bool first = true;

public:
  void prepare(double rate, double seconds) {
    line.prepare(static_cast<size_t>(rate * seconds));
    length = std::max(1, static_cast<int>(rate * .03));
    reset();
  }
  void reset() noexcept {
    line.reset();
    a = b = 0;
    remaining = 0;
    first = true;
  }
  float process(float x, float desired) noexcept {
    line.push(x);
    if (first) {
      a = b = desired;
      first = false;
    }
    if (remaining == 0 && std::abs(desired - a) > .25f) {
      b = desired;
      remaining = length;
    }
    auto tap = [&](float d) {
      return d < 1 ? x * (1 - d) + line.read(2) * d : line.read(d + 1);
    };
    const float old = tap(a);
    if (remaining == 0)
      return old;
    const float fraction = 1 - static_cast<float>(remaining) / length;
    const float y = old + (tap(b) - old) * fraction;
    if (--remaining == 0)
      a = b;
    return y;
  }
};
struct Smooth {
  float value = 0, target = 0, coefficient = .001f;
  void prepare(double sr, double seconds = .025) {
    coefficient = static_cast<float>(1 - std::exp(-1 / (sr * seconds)));
  }
  void set(float x) { target = x; }
  void snap(float x) { value = target = x; }
  float next() noexcept {
    value += coefficient * (target - value);
    return value;
  }
};
struct Biquad {
  double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  void lowpass(double sr, double frequency, double q) noexcept {
    const auto w = 2 * pi * std::clamp(frequency, 10., sr * .45) / sr;
    const auto c = std::cos(w), a = std::sin(w) / (2 * q), norm = 1 / (1 + a);
    b0 = (1 - c) * .5 * norm;
    b1 = (1 - c) * norm;
    b2 = b0;
    a1 = -2 * c * norm;
    a2 = (1 - a) * norm;
  }
  float process(float in) noexcept {
    double y = b0 * in + z1;
    z1 = b1 * in - a1 * y + z2;
    z2 = b2 * in - a2 * y;
    return clean(static_cast<float>(y));
  }
  void reset() { z1 = z2 = 0; }
};
class BandwidthFilter {
  std::array<Biquad, 4> sections;

public:
  void set(double sr, float freq) noexcept {
    for (int i = 0; i < 4; ++i)
      sections[i].lowpass(sr, freq, 1 / (2 * std::cos((2 * i + 1) * pi / 16)));
  }
  float process(float x) noexcept {
    for (auto &b : sections)
      x = b.process(x);
    return x;
  }
  void reset() {
    for (auto &b : sections)
      b.reset();
  }
};
class Allpass {
  Delay line;
  float length = 1;

public:
  void prepare(double sr, float ms) {
    line.prepare(static_cast<size_t>(sr * .02));
    length = static_cast<float>(sr * ms * .001);
  }
  float process(float x, float g) noexcept {
    float d = line.read(length);
    float y = d - g * x;
    line.push(x + g * y);
    return y;
  }
  void reset() { line.reset(); }
};
} // namespace white224
