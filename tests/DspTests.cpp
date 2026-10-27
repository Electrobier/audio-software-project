#include "dsp/ReverbEngine.h"
#include <chrono>
#include <complex>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
using namespace white224;
static thread_local bool watchAlloc = false;
static thread_local int allocations = 0;
void *operator new(std::size_t n) {
  if (watchAlloc)
    ++allocations;
  if (auto *p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
static int checks = 0;
void require(bool b, const char *msg) {
  ++checks;
  if (!b)
    throw std::runtime_error(msg);
}
std::vector<float> render(Settings s, double rate, double seconds,
                          int block = 257) {
  ReverbEngine e;
  e.setSettings(s);
  e.prepare(rate);
  const int n = static_cast<int>(rate * seconds);
  std::vector<float> interleaved(static_cast<size_t>(n) * 2);
  std::vector<float> l(static_cast<size_t>(block)),
      r(static_cast<size_t>(block));
  for (int i = 0; i < n; i += block) {
    int count = std::min(block, n - i);
    std::fill(l.begin(), l.end(), 0);
    std::fill(r.begin(), r.end(), 0);
    if (i == 0)
      l[0] = r[0] = .5f;
    e.process(l.data(), r.data(), count);
    for (int j = 0; j < count; ++j) {
      interleaved[static_cast<size_t>(2 * (i + j))] = l[static_cast<size_t>(j)];
      interleaved[static_cast<size_t>(2 * (i + j) + 1)] =
          r[static_cast<size_t>(j)];
    }
  }
  return interleaved;
}
double energy(const std::vector<float> &v) {
  double e = 0;
  for (float x : v) {
    require(std::isfinite(x), "nonfinite output");
    e += x * x;
  }
  return e;
}
int main(int argc, char **argv) {
  try {
    std::cout << "White224 DSP validation\n";
    std::filesystem::path out = argc > 1 ? argv[1] : "measurements";
    std::filesystem::create_directories(out);
    require(factoryPresets().size() == 42, "preset count");
    std::array<int, 6> counts{};
    const char *banks[] = {"Programs",    "Blade Runner",   "Halls & Spaces",
                           "Long Decays", "Plates & Drums", "Character"};
    for (auto &p : factoryPresets()) {
      for (int i = 0; i < 6; ++i)
        if (p.bank == banks[i])
          ++counts[static_cast<size_t>(i)];
      auto s = p.settings;
      s.sanitize();
      require(s.values == p.settings.values, "invalid factory values");
    }
    require(counts == std::array<int, 6>{9, 7, 5, 11, 5, 5}, "bank sizes");
    for (float x : {0.f, 1e-7f, .001f, .03f, .5f, 1.f, 4.f, -8.f})
      require(std::isfinite(VintageConverter::process(x)), "converter finite");
    require(VintageConverter::process(.123f) ==
                VintageConverter::process(.123f),
            "converter deterministic");
    std::array<double, 9> signatures{};
    const auto start = std::chrono::steady_clock::now();
    for (int p = 0; p < 9; ++p) {
      auto s = recommended(p);
      s.wet = true;
      s.values[Input] = s.values[Output] = 0;
      s.modeEnabled = s.decayEnabled = false;
      auto ir = render(s, 48000, p == 8 ? 1 : 12);
      signatures[static_cast<size_t>(p)] = energy(ir);
      require(signatures[static_cast<size_t>(p)] > 1e-6, "silent program");
      std::ofstream file(out / ("program_" + std::to_string(p + 1) + ".f32"),
                         std::ios::binary);
      file.write(reinterpret_cast<const char *>(ir.data()),
                 static_cast<std::streamsize>(ir.size() * sizeof(float)));
      std::cout << "Program " << p + 1
                << " energy=" << signatures[static_cast<size_t>(p)] << '\n';
    }
    for (int a = 0; a < 9; ++a)
      for (int b = a + 1; b < 9; ++b)
        require(std::abs(signatures[static_cast<size_t>(a)] -
                         signatures[static_cast<size_t>(b)]) > 1e-9,
                "identical programs");
    auto s = recommended(0);
    s.wet = true;
    s.modeEnabled = s.decayEnabled = false;
    s.values[Input] = s.values[Output] = 0;
    auto a = render(s, 48000, 1, 1), b = render(s, 48000, 1, 512);
    double difference = 0;
    for (size_t i = 0; i < a.size(); ++i)
      difference =
          std::max(difference, std::abs(static_cast<double>(a[i] - b[i])));
    require(difference < 1e-7, "block invariance");
    auto chorus = recommended(8);
    chorus.wet = true;
    auto c = render(chorus, 48000, .3);
    chorus.values[Bass] = 70;
    chorus.values[Mid] = 70;
    chorus.values[Crossover] = 10900;
    chorus.values[Treble] = 100;
    chorus.values[Predelay] = 255;
    require(c == render(chorus, 48000, .3),
            "chorus ignored parameters affect output");
    auto early = s;
    early.values[Predelay] = 0;
    auto late = early;
    late.values[Predelay] = 255;
    auto x = render(early, 48000, .5), y = render(late, 48000, .8);
    auto onset = [](const auto &v) {
      for (size_t i = 0; i < v.size(); i += 2)
        if (std::abs(v[i]) + std::abs(v[i + 1]) > 1e-8)
          return static_cast<int>(i / 2);
      return -1;
    };
    require(std::abs(onset(y) - onset(x) - 12240) <= 2, "predelay timing");
    auto mod = s;
    mod.modeEnabled = true;
    mod.values[Enhancement] = 16;
    require(render(mod, 48000, 1) != a, "mode enhancement no effect");
    auto opt = s;
    opt.decayEnabled = true;
    opt.values[Optimization] = 16;
    require(render(opt, 48000, 1) != a, "decay optimization no effect");
    // Silence, extrema, automation, fast program changes, block variations and
    // zero-allocation callback.
    for (double sr : {44100., 48000., 88200., 96000.})
      for (int block : {1, 17, 64, 257, 1024}) {
        ReverbEngine e;
        e.prepare(sr);
        std::vector<float> l(static_cast<size_t>(block)),
            r(static_cast<size_t>(block));
        auto t = s;
        for (int pass = 0; pass < 100; ++pass) {
          t.program = pass % 9;
          for (int j = 0; j < Count; ++j)
            t.values[j] = pass % 2 ? specs[j].min : specs[j].max;
          t.bypass = pass % 7 == 0;
          t.wet = pass % 3 == 0;
          e.setSettings(t);
          for (int j = 0; j < block; ++j) {
            l[static_cast<size_t>(j)] =
                .05f * std::sin(static_cast<float>(pass * block + j) * .13f);
            r[static_cast<size_t>(j)] = l[static_cast<size_t>(j)] * .7f;
          }
          watchAlloc = true;
          e.process(l.data(), r.data(), block);
          watchAlloc = false;
          for (int j = 0; j < block; ++j) {
            require(std::isfinite(l[static_cast<size_t>(j)]) &&
                        std::abs(l[static_cast<size_t>(j)]) < 100,
                    "unstable automation L");
            require(std::isfinite(r[static_cast<size_t>(j)]) &&
                        std::abs(r[static_cast<size_t>(j)]) < 100,
                    "unstable automation R");
          }
        }
      }
    require(allocations == 0, "allocations in callback");
    ReverbEngine by;
    auto bs = s;
    bs.bypass = true;
    by.setSettings(bs);
    by.prepare(48000);
    float l[16], r[16];
    std::fill_n(l, 16, .123f);
    std::fill_n(r, 16, -.234f);
    by.process(l, r, 16);
    for (int i = 0; i < 16; ++i)
      require(l[i] == .123f && r[i] == -.234f, "bypass not unity dry");
    ReverbEngine silence;
    silence.prepare(48000);
    float zl[256]{}, zr[256]{};
    silence.process(zl, zr, 256);
    for (int i = 0; i < 256; ++i)
      require(zl[i] == 0 && zr[i] == 0, "silence noise");
    zl[0] = std::numeric_limits<float>::quiet_NaN();
    zr[0] = std::numeric_limits<float>::infinity();
    silence.process(zl, zr, 256);
    for (int i = 0; i < 256; ++i)
      require(std::isfinite(zl[i]) && std::isfinite(zr[i]), "NaN guard");

    // Every exposed fader changes its actual audio path (not just its display).
    for (int parameter = 0; parameter < Count; ++parameter) {
      auto low = s, high = s;
      if (parameter == Mix)
        low.wet = high.wet = false;
      low.values[parameter] = specs[parameter].min;
      high.values[parameter] = specs[parameter].max;
      if (parameter == Enhancement)
        low.modeEnabled = high.modeEnabled = true;
      if (parameter == Optimization)
        low.decayEnabled = high.decayEnabled = true;
      require(render(low, 48000, 1) != render(high, 48000, 1),
              "ineffective fader");
    }
    std::ofstream filterReport(out / "bandwidth.csv");
    filterReport
        << "sample_rate,requested_hz,effective_hz,response_db_at_cutoff\n";
    for (double sr : {44100., 48000., 88200., 96000.})
      for (float cutoff : {3000.f, 8100.f, 20000.f}) {
        BandwidthFilter filter;
        filter.set(sr, cutoff);
        std::complex<double> response{};
        const double effective = std::min(double(cutoff), sr * .45);
        for (int n = 0; n < 32768; ++n) {
          float output = filter.process(n == 0 ? 1.f : 0.f);
          response +=
              double(output) * std::polar(1., -2 * pi * effective * n / sr);
        }
        const auto db = 20 * std::log10(std::abs(response));
        require(std::abs(db + 3.0103) < .02, "bandwidth cutoff response");
        filterReport << sr << ',' << cutoff << ',' << effective << ',' << db
                     << '\n';
      }
    auto wetSettings = s;
    wetSettings.wet = true;
    wetSettings.values[Mix] = 0;
    auto wetZero = render(wetSettings, 48000, .5);
    wetSettings.values[Mix] = 100;
    require(wetZero == render(wetSettings, 48000, .5),
            "100 percent wet changes stored mix influence");
    std::ofstream cpu(out / "cpu.csv");
    cpu << "block,steady_audio_seconds,processing_seconds,realtime_percent,max_"
           "observed_callback_us\n";
    for (int size : {64, 256, 1024}) {
      ReverbEngine bench;
      bench.setSettings(s);
      bench.prepare(48000);
      std::vector<float> left(static_cast<size_t>(size)),
          right(static_cast<size_t>(size));
      double maxMicros = 0;
      auto begin = std::chrono::steady_clock::now();
      int processed = 0;
      for (int offset = 0; offset < 48000 * 3; offset += size) {
        std::fill(left.begin(), left.end(), .01f);
        std::fill(right.begin(), right.end(), -.005f);
        auto tick = std::chrono::steady_clock::now();
        bench.process(left.data(), right.data(), size);
        maxMicros =
            std::max(maxMicros, std::chrono::duration<double, std::micro>(
                                    std::chrono::steady_clock::now() - tick)
                                    .count());
        processed += size;
      }
      auto elapsed = std::chrono::duration<double>(
                         std::chrono::steady_clock::now() - begin)
                         .count();
      cpu << size << ',' << processed / 48000. << ',' << elapsed << ','
          << elapsed / (processed / 48000.) * 100 << ',' << maxMicros << '\n';
    }
    CrossfadeDelay changingDelay;
    changingDelay.prepare(48000, .3);
    float previous = 0;
    for (int i = 0; i < 48000; ++i) {
      float input =
          .5f * std::sin(static_cast<float>(2 * pi * 1000 * i / 48000));
      float desired = i < 10000 ? 0.f : i < 24000 ? 12240.f : 4800.f;
      float sample = changingDelay.process(input, desired);
      if (i > 1)
        require(std::abs(sample - previous) < .08f,
                "delay crossfade discontinuity");
      previous = sample;
    }
    auto longest = s;
    longest.values[Bass] = longest.values[Mid] = 70;
    longest.values[Treble] = 10900;
    auto longir = render(longest, 48000, 90, 512);
    require(energy(longir) > 1e-6, "70 second silence");
    std::ofstream lf(out / "seventy_seconds.f32", std::ios::binary);
    lf.write(reinterpret_cast<const char *>(longir.data()),
             static_cast<std::streamsize>(longir.size() * sizeof(float)));
    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
            .count();
    std::cout << "PASS " << checks
              << " assertions, callback allocations=" << allocations
              << ", full suite seconds=" << seconds << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
