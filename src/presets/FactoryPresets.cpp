#include "FactoryPresets.h"
namespace white224 {
const std::array<Program, 9> &programs() {
  static const std::array<Program, 9> p{
      {{"Small Concert Hall B",
        {29.7f, 37.1f, 41.3f, 43.7f, 47.9f, 53.3f, 59.3f, 61.1f},
        4,
        .68f,
        .24f,
        .65f,
        .7f,
        0},
       {"Vocal Plate",
        {23.3f, 29.9f, 31.7f, 37.3f, 41.9f, 43.1f, 47.3f, 53.9f},
        6,
        .74f,
        .12f,
        .48f,
        .42f,
        1},
       {"Large Concert Hall B",
        {43.3f, 53.9f, 61.3f, 67.1f, 71.9f, 79.3f, 83.9f, 89.3f},
        5,
        .71f,
        .3f,
        .8f,
        .78f,
        2},
       {"Acoustic Chamber",
        {19.9f, 23.9f, 29.3f, 31.1f, 37.9f, 41.3f, 43.9f, 47.9f},
        3,
        .63f,
        .42f,
        .36f,
        .58f,
        3},
       {"Percussion Plate A",
        {17.9f, 23.3f, 29.9f, 31.7f, 37.3f, 41.9f, 43.1f, 47.3f},
        6,
        .77f,
        .1f,
        .23f,
        .35f,
        4},
       {"Small Concert Hall A",
        {31.3f, 37.9f, 43.3f, 47.3f, 53.9f, 59.9f, 61.3f, 67.1f},
        4,
        .65f,
        .28f,
        .57f,
        .65f,
        5},
       {"Room A",
        {11.3f, 13.9f, 17.3f, 19.9f, 23.3f, 29.3f, 31.1f, 37.9f},
        2,
        .56f,
        .55f,
        .17f,
        .88f,
        6},
       {"Constant Density Plate A",
        {23.9f, 29.3f, 31.1f, 37.9f, 41.3f, 43.9f, 47.9f, 53.3f},
        6,
        .79f,
        .08f,
        .41f,
        .3f,
        7},
       {"Chorus A", {20, 23, 27, 31, 0, 0, 0, 0}, 0, 0, 0, 1, 0, 0}}};
  return p;
}
Settings recommended(int p) {
  static constexpr float data[8][5] = {
      {2.83f, 2.27f, 433, 4300, 4},  {1.94f, 2.14f, 813, 10900, 3.5f},
      {3.34f, 3.05f, 406, 3380, 6},  {2.83f, 2.37f, 297, 6050, 6},
      {2.07f, 2.15f, 241, 10900, 2}, {3.08f, 2.2f, 488, 5400, 4},
      {1.25f, 1.37f, 331, 3810, 4},  {1.54f, 1.72f, 1231, 10900, 1.5f}};
  Settings s;
  s.program = std::clamp(p, 0, 8);
  if (p >= 0 && p < 8) {
    for (int i = 0; i < 4; ++i)
      s.values[i] = data[p][i];
    s.values[Predelay] = data[p][4];
  }
  return s;
}
const std::array<Preset, 42> &factoryPresets() {
  static const auto result = [] {
    std::array<Preset, 42> a{};
    int n = 0;
    for (int i = 0; i < 9; ++i)
      a[n++] = {"Programs", programs()[i].name, recommended(i), false};
    // All following names and parameter choices are reconstructed, not
    // extracted.
    auto add = [&](const char *bank, const char *name, int p, float time,
                   float dark, float depth, float pre, float mix, float mod) {
      auto s = recommended(p);
      s.values[Bass] = std::min(70.f, time * 1.13f);
      s.values[Mid] = time;
      s.values[Treble] = dark;
      s.values[Depth] = depth;
      s.values[Predelay] = pre;
      s.values[Mix] = mix;
      s.values[Enhancement] = mod;
      a[n++] = {bank, name, s, true};
    };
    add("Blade Runner", "Neon Rain", 2, 9, 2400, 61, 65, 45, 7);
    add("Blade Runner", "Off World", 0, 14, 1800, 71, 110, 50, 9);
    add("Blade Runner", "Memory Room", 3, 5.5f, 3200, 45, 36, 40, 5);
    add("Blade Runner", "Replicant Dreams", 5, 18, 2100, 65, 95, 52, 11);
    add("Blade Runner", "City at Midnight", 2, 12, 3800, 57, 78, 48, 6);
    add("Blade Runner", "Distant Piano", 0, 7, 4100, 65, 125, 38, 4);
    add("Blade Runner", "Warm Future", 3, 8, 2700, 51, 43, 44, 8);
    add("Halls & Spaces", "Orchestra", 2, 3.2f, 6200, 48, 24, 32, 4);
    add("Halls & Spaces", "Wooden Hall", 5, 2.6f, 3800, 39, 18, 32, 3);
    add("Halls & Spaces", "Warm Chamber", 3, 1.8f, 4200, 27, 11, 28, 3);
    add("Halls & Spaces", "Drum Room", 6, .7f, 5400, 12, 2, 20, 2);
    add("Halls & Spaces", "Cathedral", 2, 7.5f, 4900, 69, 46, 42, 5);
    const char *names[] = {"Ten Seconds",
                           "Fifteen Seconds",
                           "Twenty Seconds",
                           "Thirty Seconds",
                           "Forty Seconds",
                           "Fifty Seconds",
                           "Seventy Seconds (finite)",
                           "Dark Drone",
                           "Slow Bloom",
                           "Endless Night",
                           "Silver Cloud"};
    const float times[] = {10, 15, 20, 30, 40, 50, 70, 35, 25, 60, 45};
    for (int i = 0; i < 11; ++i)
      add("Long Decays", names[i], i % 2 ? 2 : 7, times[i],
          i == 6 ? 10900.f : 1800.f + 240 * i, 60, 20.f + 8 * i, 50,
          4.f + i * .6f);
    add("Plates & Drums", "Dense Vocal", 1, 2.4f, 8500, 31, 28, 32, 4);
    add("Plates & Drums", "Eighties Snare", 4, 1.6f, 7800, 18, 16, 28, 2);
    add("Plates & Drums", "Short Snare", 4, .65f, 6400, 5, 4, 25, 1);
    add("Plates & Drums", "Bright Plate", 7, 2.1f, 10900, 20, 9, 33, 5);
    add("Plates & Drums", "Dark Percussion", 4, 1.3f, 2800, 24, 7, 30, 3);
    add("Character", "Floating Hall", 0, 6, 4800, 55, 30, 44, 16);
    add("Character", "Lo Fi Chamber", 3, 3, 1900, 35, 19, 40, 6);
    add("Character", "Wide Chorus", 8, 2, 4300, 60, 4, 42, 6);
    add("Character", "Moving Plate", 7, 4, 6100, 47, 24, 39, 14);
    add("Character", "Small and Warm", 6, 1.1f, 2200, 15, 5, 25, 3);
    return a;
  }();
  return result;
}
} // namespace white224
