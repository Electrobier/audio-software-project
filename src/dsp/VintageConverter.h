#pragma once
#include <algorithm>
#include <cmath>
namespace white224 {
// Approximation: four ranges separated by 18 dB, 12-bit signed mantissa.
// Above full scale the range expands rather than introducing hard clipping.
// No claim to match the historical converter transfer curve.
struct VintageConverter {
  static float process(float x) noexcept {
    if (!std::isfinite(x))
      return 0;
    const float a = std::abs(x);
    if (a < 1.e-12f)
      return 0;
    float range = a <= .001953125f ? .001953125f
                  : a <= .015625f  ? .015625f
                  : a <= .125f     ? .125f
                                   : 1.f;
    if (a > 1)
      range = std::exp2(std::ceil(std::log2(a)));
    const float step = range / 2047.f;
    return std::round(x / step) * step;
  }
};
} // namespace white224
