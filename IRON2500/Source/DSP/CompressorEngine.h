// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace iron
{
struct Parameters
{
    double threshold = -10, attack = 3, ratio = 2, release = .2, variable = 0, knee = 6;
    int thrust = 0, type = 0, shape = 3;
    double link = 1, analog = 0, output = 0, mix = 1, trim = 0, bypass = 0;
};
inline double db(double x)
{
    return 20 * std::log10(std::max(1e-12, std::abs(x)));
}
inline double gain(double x)
{
    return std::pow(10., x / 20.);
}
inline double reduction(double level, double threshold, double ratio, double width)
{
    const auto x = level - threshold, slope = 1. - 1. / ratio;
    if (width <= 0)
        return std::max(0., x) * slope;
    if (x <= -width * .5)
        return 0;
    if (x >= width * .5)
        return x * slope;
    return slope * (x + width * .5) * (x + width * .5) / (2 * width);
}
struct OnePole
{
    double z = 0;
    double low(double x, double a)
    {
        z += a * (x - z);
        if (std::abs(z) < 1e-24)
            z = 0;
        return z;
    }
};
struct Delay
{
    std::array<double, 8> data{};
    size_t index = 0;
    double tick(double x)
    {
        auto y = data[index];
        data[index] = x;
        index = (index + 1) % 8;
        return y;
    }
};
// Two 17-tap linear-phase FIR stages at 2x. Round-trip latency: 8 base samples.
struct Saturator
{
    std::array<double, 17> taps{}, up{}, down{};
    size_t posUp = 0, posDown = 0;
    void prepare()
    {
        double sum = 0;
        for (int i = 0; i < 17; ++i)
        {
            double n = i - 8;
            taps[static_cast<size_t>(i)] =
                (i == 8 ? .5 : std::sin(std::numbers::pi * .5 * n) / (std::numbers::pi * n)) *
                (.42 - .5 * std::cos(2 * std::numbers::pi * i / 16) +
                 .08 * std::cos(4 * std::numbers::pi * i / 16));
            sum += taps[static_cast<size_t>(i)];
        }
        for (auto &t : taps)
            t /= sum;
        up.fill(0);
        down.fill(0);
        posUp = posDown = 0;
    }
    double fir(double x, std::array<double, 17> &b, size_t &p)
    {
        b[p] = x;
        double y = 0;
        size_t j = p;
        for (auto t : taps)
        {
            y += t * b[j];
            j = j == 0 ? 16 : j - 1;
        }
        p = (p + 1) % 17;
        return y;
    }
    double tick(double x)
    {
        double y = 0;
        for (int i = 0; i < 2; ++i)
        {
            double v = 2 * fir(i == 0 ? x : 0, up, posUp);
            // Odd transfer: no generated DC, unit small-signal gain.
            double s = std::tanh(.8 * v) / .8;
            auto d = fir(s, down, posDown);
            if (i == 0)
                y = d;
        }
        return y;
    }
};
class CompressorEngine
{
  public:
    static constexpr int latency = 8;
    void prepare(double rate)
    {
        sr = std::clamp(rate, 8000., 384000.);
        smooth = 1 - std::exp(-1 / (.01 * sr));
        meterA = 1 - std::exp(-1 / (.3 * sr));
        for (auto &c : channels)
        {
            c = {};
            c.sat.prepare();
        }
        current = Parameters{};
        initial = true;
    }
    // The engine owns no heap storage and handles arbitrary host block sizes.
    void setParameters(const Parameters &p)
    {
        target = p;
        if (initial)
        {
            current = p;
            initial = false;
        }
    }
    void process(double &left, double &right, int channelCount = 2)
    {
        step();
        double raw[2] = {finite(left), finite(right)}, in[2], level[2], control[2];
        const double aLow = 1 - std::exp(-2 * std::numbers::pi * 180 / sr);
        const double aHigh = 1 - std::exp(-2 * std::numbers::pi * 2500 / sr);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto &c = channels[static_cast<size_t>(ch)];
            in[ch] = raw[ch] * gain(current.trim);
            const double fb = target.type == 0 ? in[ch] : c.feedback;
            const double lo = c.low.low(fb, aLow), hi = c.high.low(fb, aHigh);
            // Shelving tilt only in sidechain. Normal, moderate, strong.
            const double tilted = target.thrust == 0   ? fb
                                  : target.thrust == 1 ? fb - .55 * lo + .25 * (fb - hi)
                                                       : fb - .85 * lo + .65 * (fb - hi);
            const double sl = c.shapeLow.low(tilted, aLow), sh = c.shapeHigh.low(tilted, aHigh);
            const double shaped = target.shape == 0   ? sh - sl
                                  : target.shape == 1 ? tilted - sl
                                  : target.shape == 2 ? sl
                                                      : tilted;
            const double power = c.power.low(shaped * shaped, 1 - std::exp(-1 / (.004 * sr)));
            const double detected = target.type == 2 ? std::sqrt(std::max(0., power)) : std::abs(shaped);
            level[ch] = db(detected);
        }
        if (channelCount == 1)
            level[1] = level[0];
        const auto linked = std::max(level[0], level[1]);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto &c = channels[static_cast<size_t>(ch)];
            control[ch] = level[ch] + current.link * (linked - level[ch]);
            double desired = reduction(control[ch], current.threshold, current.ratio, current.knee);
            double slope = 1 - 1 / current.ratio;
            if (target.type != 0)
                desired *= current.ratio; // Feedback law uses (R-1).
            desired = std::clamp(desired, 0., 120.);
            const double adaptive = c.history.low(c.gr, 1 - std::exp(-1 / (.4 * sr)));
            double release = current.release * (1 + current.variable * std::min(3., adaptive / 6));
            double time = desired > c.gr ? current.attack * .001 : release;
            if (target.type == 2)
                time *= 1.4;
            double alpha = 1 - std::exp(-1 / (std::max(1e-6, time) * sr));
            // Implicit feedback relaxation avoids high-ratio/fast-attack instability.
            if (target.type != 0)
                alpha /= 1 + alpha * slope * current.ratio;
            c.gr += alpha * (desired - c.gr);
            if (c.gr < 1e-20)
                c.gr = 0;
            double wet = in[ch] * gain(-c.gr);
            c.feedback = wet;
            const double clean = c.clean.tick(wet), colored = c.sat.tick(wet), dry = c.dry.tick(raw[ch]);
            wet = clean + (colored - clean) * current.analog;
            double out = (dry + (wet - dry) * current.mix) * gain(current.output);
            out += current.bypass * (dry - out);
            c.inputMeter.low(raw[ch] * raw[ch], meterA);
            c.outputMeter.low(out * out, meterA);
            c.grMeter.low(c.gr, meterA);
            if (ch == 0)
                left = finite(out);
            else
                right = finite(out);
        }
    }
    double meter(int ch, int mode) const
    {
        const auto &c = channels[static_cast<size_t>(std::clamp(ch, 0, 1))];
        if (mode == 0)
            return c.grMeter.z;
        return db(std::sqrt(std::max(0., mode == 1 ? c.outputMeter.z : c.inputMeter.z)));
    }
    double gainReduction(int ch) const
    {
        return channels[static_cast<size_t>(ch)].gr;
    }

  private:
    struct Channel
    {
        OnePole low, high, shapeLow, shapeHigh, power, history, inputMeter, outputMeter, grMeter;
        double gr = 0, feedback = 0;
        Delay clean, dry;
        Saturator sat;
    };
    std::array<Channel, 2> channels{};
    Parameters current, target;
    double sr = 48000, smooth = .002, meterA = .0001;
    bool initial = true;
    static double finite(double x)
    {
        return std::isfinite(x) ? std::clamp(x, -1e6, 1e6) : 0;
    }
    void step()
    {
        auto s = [this](double &x, double t) { x += smooth * (t - x); };
        s(current.threshold, target.threshold);
        s(current.attack, target.attack);
        s(current.ratio, target.ratio);
        s(current.release, target.release);
        s(current.variable, target.variable);
        s(current.knee, target.knee);
        s(current.link, target.link);
        s(current.analog, target.analog);
        s(current.output, target.output);
        s(current.mix, target.mix);
        s(current.trim, target.trim);
        s(current.bypass, target.bypass);
    }
};
} // namespace iron
