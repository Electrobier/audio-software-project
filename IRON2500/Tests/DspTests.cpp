// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#include "DSP/CompressorEngine.h"
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
using namespace iron;
int checks = 0;
void check(bool ok, const char *text)
{
    ++checks;
    if (!ok)
        throw std::runtime_error(text);
}
void near(double a, double b, double tol, const char *text)
{
    check(std::abs(a - b) <= tol, text);
}
void tick(CompressorEngine &e, double x, double y, int n)
{
    for (int i = 0; i < n; ++i)
    {
        double l = x, r = y;
        e.process(l, r);
    }
}
int main()
{
    try
    {
        for (double r : {1.5, 2., 3., 4., 6., 10., 1000.})
            for (double x : {-30., -20., -10., 0., 10.})
                near(reduction(x, -10, r, 0), std::max(0., x + 10) * (1 - 1 / r), 1e-10,
                     "ratio / threshold curve");
        for (double k : {0., 6., 12.})
        {
            double prev = 0;
            for (int i = -400; i <= 200; ++i)
            {
                double y = reduction(i * .1, -10, 4, k);
                check(y >= prev - 1e-12, "monotonic knee");
                prev = y;
            }
            if (k > 0)
            {
                near(reduction(-10 - k / 2, -10, 4, k), 0, 1e-12, "knee lower boundary");
                near(reduction(-10 + k / 2, -10, 4, k), k * .375, 1e-12, "knee upper boundary");
            }
        }
        for (double sr : {44100., 48000., 88200., 96000., 176400., 192000.})
        {
            for (double attack : {.03, .1, .3, 1., 3., 10., 30.})
            {
                Parameters p;
                p.threshold = -20;
                p.ratio = 4;
                p.knee = 0;
                p.attack = attack;
                CompressorEngine e;
                e.prepare(sr);
                e.setParameters(p);
                int n = (int)std::ceil(attack * .001 * sr);
                tick(e, 1, 1, n);
                double expected = 15 * (1 - std::exp(-n / (attack * .001 * sr)));
                near(e.gainReduction(0), expected, 1e-7, "measured attack time");
                std::cout << "attack sr=" << sr << " target_ms=" << attack << " sample_ms=" << 1000 * n / sr
                          << " GR=" << e.gainReduction(0) << " expected=" << expected << "\n";
            }
            for (double release : {.05, .1, .2, .5, 1., 2.})
            {
                Parameters p;
                p.threshold = -20;
                p.ratio = 4;
                p.knee = 0;
                p.attack = .03;
                p.release = release;
                CompressorEngine e;
                e.prepare(sr);
                e.setParameters(p);
                tick(e, 1, 1, (int)(.1 * sr));
                int n = (int)std::ceil(sr * release);
                tick(e, 0, 0, n);
                near(e.gainReduction(0), 15 * std::exp(-n / (release * sr)), 1e-7, "measured release time");
                std::cout << "release sr=" << sr << " target_s=" << release << " GR=" << e.gainReduction(0)
                          << "\n";
            }
        }
        for (double link : {0., .35, 1.})
        {
            Parameters p;
            p.threshold = -20;
            p.knee = 0;
            p.attack = .03;
            p.release = .05;
            p.link = link;
            CompressorEngine e;
            e.prepare(48000);
            e.setParameters(p);
            tick(e, 1, .01, 48000);
            near(e.gainReduction(0), 10, 1e-7, "left compression");
            near(e.gainReduction(1), std::max(0., -40 + 40 * link + 20) * .5, 1e-7, "stereo linking");
        }
        for (int type = 0; type < 3; ++type)
        {
            Parameters p;
            p.type = type;
            p.threshold = -20;
            p.ratio = 4;
            p.knee = 0;
            p.attack = 3;
            CompressorEngine e;
            e.prepare(48000);
            e.setParameters(p);
            tick(e, 1, 1, 48000);
            near(e.gainReduction(0), 15, .03, "feedback steady state ratio");
        }
        std::mt19937 rng(2500);
        std::uniform_real_distribution<double> noise(-.5, .5);
        for (int mode = 0; mode < 3; ++mode)
        {
            CompressorEngine e;
            e.prepare(48000);
            Parameters p;
            p.threshold = 10;
            p.trim = mode == 1 ? 0 : 6;
            p.bypass = mode == 0 ? 1 : 0;
            p.mix = mode == 1 ? 1 : 0;
            e.setParameters(p);
            std::array<double, 8> hist{};
            for (int i = 0; i < 24000; ++i)
            {
                double in = noise(rng), l = in, r = in;
                e.process(l, r);
                near(l, hist[(size_t)i % 8], 1e-12, "bypass / clean / dry null");
                hist[(size_t)i % 8] = in;
            }
        }
        {
            CompressorEngine e;
            e.prepare(48000);
            Parameters p;
            p.threshold = 10;
            p.output = 6;
            e.setParameters(p);
            tick(e, .1, .1, 48000);
            double l = .1, r = .1;
            e.process(l, r);
            near(l, .1 * gain(6), 1e-10, "output gain");
        }
        {
            CompressorEngine e;
            e.prepare(48000);
            Parameters p;
            p.threshold = 10;
            p.analog = 1;
            e.setParameters(p);
            tick(e, .8, .8, 48000);
            double l = .8, r = .8;
            e.process(l, r);
            check(l > 0 && l < .8, "analog saturation compresses amplitude");
            near(l, std::tanh(.64) / .8, 1e-6, "saturation DC transfer");
        }
        {
            CompressorEngine e;
            e.prepare(48000);
            Parameters p;
            p.threshold = 10;
            e.setParameters(p);
            tick(e, .1, .1, 1000);
            p.output = 24;
            e.setParameters(p);
            double last = .1;
            for (int i = 0; i < 5000; ++i)
            {
                double l = .1, r = .1;
                e.process(l, r);
                check(std::abs(l - last) < .01, "gain smoothing continuity");
                last = l;
            }
        }
        {
            CompressorEngine e;
            Parameters p;
            for (double sr : {44100., 192000., 48000.})
            {
                e.prepare(sr);
                for (int type = 0; type < 3; ++type)
                    for (int shape = 0; shape < 4; ++shape)
                        for (int thrust = 0; thrust < 3; ++thrust)
                        {
                            p.type = type;
                            p.shape = shape;
                            p.thrust = thrust;
                            p.ratio = 1000;
                            p.attack = .03;
                            p.analog = 1;
                            e.setParameters(p);
                            for (int n = 0; n < 8192; ++n)
                            {
                                double l = noise(rng), r = noise(rng);
                                e.process(l, r);
                                check(std::isfinite(l) && std::isfinite(r), "all modes finite");
                            }
                        }
            }
            double l = std::nan(""), r = INFINITY;
            e.process(l, r);
            check(std::isfinite(l) && std::isfinite(r), "invalid input safety");
        }
        // Block grouping cannot affect engine results.
        {
            CompressorEngine a, b;
            a.prepare(48000);
            b.prepare(48000);
            Parameters p;
            p.analog = 1;
            a.setParameters(p);
            b.setParameters(p);
            std::vector<double> input(8192), expected;
            for (auto &x : input)
                x = noise(rng);
            for (auto x : input)
            {
                double l = x, r = x;
                a.process(l, r);
                expected.push_back(l);
            }
            size_t index = 0;
            for (int block : {1, 7, 64, 511, 2048, 5561})
                for (int n = 0; n < block; ++n)
                {
                    double l = input[index], r = l;
                    b.process(l, r);
                    near(l, expected[index], 1e-12, "block-size independence");
                    ++index;
                }
        }
        // Audio probes verify that named detector modes actually change response.
        auto sineGR = [](int thrust, int shape, double frequency)
        {
            CompressorEngine engine;
            engine.prepare(48000);
            Parameters p;
            p.threshold = -20;
            p.knee = 0;
            p.attack = .3;
            p.release = .1;
            p.thrust = thrust;
            p.shape = shape;
            engine.setParameters(p);
            double sum = 0;
            for (int n = 0; n < 96000; ++n)
            {
                double l = .8 * std::sin(2 * std::numbers::pi * frequency * n / 48000), r = l;
                engine.process(l, r);
                if (n >= 48000)
                    sum += engine.gainReduction(0);
            }
            return sum / 48000;
        };
        double bassNormal = sineGR(0, 3, 60), bassLoud = sineGR(2, 3, 60);
        double highNormal = sineGR(0, 3, 6000), highLoud = sineGR(2, 3, 6000);
        check(bassLoud < bassNormal - 2, "Thrust reduces bass detector dominance");
        check(highLoud > highNormal + .5, "Thrust increases high frequency weighting");
        check(sineGR(0, 1, 60) < sineGR(0, 2, 60), "HP/LP detector distinction");
        std::cout << "sine 60Hz GR normal=" << bassNormal << " loud=" << bassLoud
                  << "; 6kHz normal=" << highNormal << " loud=" << highLoud << "\n";
        double tails[2]{};
        for (int variable = 0; variable < 2; ++variable)
        {
            CompressorEngine engine;
            engine.prepare(48000);
            Parameters p;
            p.threshold = -20;
            p.knee = 0;
            p.attack = .03;
            p.release = .1;
            p.variable = variable;
            engine.setParameters(p);
            tick(engine, 1, 1, 96000);
            tick(engine, 0, 0, 4800);
            tails[variable] = engine.gainReduction(0);
        }
        check(tails[1] > tails[0] + 1, "program-dependent release extends sustained recovery");
        // Impulse and finite zero tail, with all topologies and saturation.
        for (int type = 0; type < 3; ++type)
        {
            CompressorEngine engine;
            engine.prepare(48000);
            Parameters p;
            p.type = type;
            p.analog = 1;
            engine.setParameters(p);
            double energy = 0;
            for (int n = 0; n < 48000; ++n)
            {
                double l = n == 0 ? 1 : 0, r = l;
                engine.process(l, r);
                check(std::isfinite(l), "impulse finite");
                energy += l * l;
            }
            check(energy > 0 && energy < 2, "impulse processed and bounded");
        }
        std::cout << "PASS: " << checks << " assertions\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FAIL: " << e.what() << " after " << checks << " assertions\n";
        return 1;
    }
}
