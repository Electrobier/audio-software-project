#include "PluginEditor.h"
using namespace white224;
namespace {
const juce::Colour ink(0xff252626), cream(0xffe6e2d5), red(0xffff3327);
juce::Font font(float size, bool bold = false) {
  return juce::Font(
      juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
}
void led(juce::Graphics &g, float x, float y, bool on) {
  if (on) {
    g.setColour(red.withAlpha(.16f));
    g.fillEllipse(x - 4, y - 4, 15, 15);
  }
  g.setColour(on ? red : juce::Colour(0xff661b16));
  g.fillEllipse(x, y, 7, 7);
  if (on) {
    g.setColour(juce::Colour(0xffffa187));
    g.fillEllipse(x + 1, y + .6f, 3, 2);
  }
}
} // namespace
void PanelLook::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                     const juce::Colour &, bool hover,
                                     bool down) {
  auto r = b.getLocalBounds().toFloat().reduced(2);
  const bool round = b.getProperties().getWithDefault("round", false);
  float radius = round ? r.getHeight() * .5f : 3.f;
  if (realistic) {
    g.setColour(juce::Colours::black.withAlpha(.3f));
    g.fillRoundedRectangle(r.translated(1, 3), radius);
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(down ? 0xff111313 : 0xff454646), r.getTopLeft(),
        juce::Colour(0xff111212), r.getBottomLeft(), false));
  } else
    g.setColour(juce::Colour(down ? 0xff111212 : 0xff272929));
  g.fillRoundedRectangle(r, radius);
  g.setColour(juce::Colours::white.withAlpha(hover ? .4f : .12f));
  g.drawRoundedRectangle(r.reduced(.5f), radius, 1);
  if (b.getProperties().getWithDefault("led", true))
    led(g, r.getRight() - 13, r.getY() + 7, b.getToggleState() || b.isDown());
}
void PanelLook::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool,
                               bool down) {
  g.setFont(font(b.getHeight() > 45 ? 22.f : 11.5f, true));
  g.setColour(juce::Colour(0xffeeeadd));
  g.drawFittedText(b.getButtonText(),
                   b.getLocalBounds().reduced(6).translated(0, down ? 1 : 0),
                   juce::Justification::centred, 2);
}
void PanelLook::drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h,
                                 float pos, float, float,
                                 juce::Slider::SliderStyle, juce::Slider &) {
  const float cx = x + w * .5f;
  g.setColour(juce::Colour(0xffb4b0a5));
  g.fillRoundedRectangle(cx - 6, static_cast<float>(y - 3), 12,
                         static_cast<float>(h + 6), 4);
  g.setColour(juce::Colour(0xff171818));
  g.fillRoundedRectangle(cx - 3, static_cast<float>(y), 6,
                         static_cast<float>(h), 2);
  for (int i = 0; i <= 10; ++i) {
    float sy = y + h * i * .1f;
    g.setColour(juce::Colour(0xff716f67));
    g.drawLine(cx - 28, sy, cx - 13, sy, .7f);
    g.drawLine(cx + 13, sy, cx + 28, sy, .7f);
  }
  auto cap = juce::Rectangle<float>(cx - 24, pos - 12, 48, 24);
  if (realistic) {
    g.setColour(juce::Colours::black.withAlpha(.35f));
    g.fillRoundedRectangle(cap.translated(2, 4), 2);
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff606361), cap.getTopLeft(), juce::Colour(0xff222625),
        cap.getBottomLeft(), false));
  } else
    g.setColour(juce::Colour(0xff383c39));
  g.fillRoundedRectangle(cap, 2);
  g.setColour(juce::Colour(0xff8a8b80));
  g.drawRoundedRectangle(cap, 2, 1);
  g.setColour(cream);
  g.fillRect(cx - 20, pos - 1, 40.f, 2.f);
}
White224Editor::White224Editor(White224Processor &p)
    : AudioProcessorEditor(p), processor(p) {
  setLookAndFeel(&look);
  addAndMakeVisible(panel);
  panel.setInterceptsMouseClicks(false, true);
  look.realistic = p.uiFlag("realistic", true);
  shift = p.uiFlag("shift", false);
  call = p.uiFlag("call", true);
  auto add = [&](juce::Component &c) { panel.addAndMakeVisible(c); };
  add(presets);
  add(save);
  add(load);
  add(design);
  refreshPresets();
  presets.onChange = [this] {
    const auto i = presets.getSelectedId();
    if (i >= 1 && i <= 42) {
      processor.setCurrentProgram(i - 1);
      lastFactory = i - 1;
    } else if (i >= 100 && i < 100 + userFiles.size()) {
      auto r = processor.loadPreset(userFiles[i - 100]);
      if (r.failed())
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon, "Preset",
            r.getErrorMessage());
    }
  };
  save.onClick = [this] { chooseFile(true); };
  load.onClick = [this] { chooseFile(false); };
  design.onClick = [this] {
    look.realistic = !look.realistic;
    persist();
    repaint();
  };
  for (int i = 0; i < 8; ++i) {
    auto &b = programs[i];
    add(b);
    b.setButtonText(juce::String(i + 1));
    b.setTooltip(::white224::programs()[i].name);
    b.onClick = [this, i] {
      if (shift && i == 6)
        processor.setParameter("modeEnabled",
                               processor.settings().modeEnabled ? 0.f : 1.f);
      else if (shift && i == 7)
        processor.setParameter("decayEnabled",
                               processor.settings().decayEnabled ? 0.f : 1.f);
      else
        processor.selectAlgorithm(shift && i == 0 ? 8 : i, call);
    };
  }
  const char *labels[] = {"IMMED", "SET", "CALL", "SHIFT", "A", "B", "C", "D"};
  for (int i = 0; i < 8; ++i) {
    auto &b = functions[i];
    add(b);
    b.setButtonText(labels[i]);
    b.onClick = [this, i] {
      if (i == 0) {
        call = false;
        persist();
      } else if (i == 1)
        armed = !armed;
      else if (i == 2) {
        call = true;
        persist();
      } else if (i == 3) {
        shift = !shift;
        held = -1;
        bindFaders();
        persist();
      } else {
        int r = i - 4;
        if (armed) {
          processor.storeRegister(r);
          armed = false;
          regFlash = r;
          flashTicks = 12;
        } else if (processor.recallRegister(r)) {
          regFlash = r;
          flashTicks = 12;
        }
      }
      repaint();
    };
  }
  for (int i = 0; i < 6; ++i) {
    add(faders[i]);
    faders[i].setSliderStyle(juce::Slider::LinearVertical);
    faders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    faders[i].setScrollWheelEnabled(true);
    add(values[i]);
    values[i].setButtonText("VALUE");
    values[i].onStateChange = [this, i] {
      if (values[i].isDown())
        held = i;
      else if (held == i)
        held = -1;
      repaint();
    };
  }
  wet.setClickingTogglesState(true);
  bypass.setClickingTogglesState(true);
  add(wet);
  add(bypass);
  wet.getProperties().set("round", true);
  bypass.getProperties().set("round", true);
  wetAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.state, "wet", wet);
  bypassAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.state, "bypass", bypass);
  bindFaders();
  setResizable(true, true);
  getConstrainer()->setFixedAspectRatio(900. / 780.);
  setResizeLimits(630, 546, 1440, 1248);
  auto size = p.uiSize();
  setSize(size[0], size[1]);
  timerCallback();
  startTimerHz(30);
}
White224Editor::~White224Editor() {
  stopTimer();
  persist();
  setLookAndFeel(nullptr);
}
void White224Editor::persist() {
  processor.setUi(getWidth(), getHeight(), look.realistic, shift, call);
}
void White224Editor::bindFaders() {
  for (auto &a : attachments)
    a.reset();
  for (int i = 0; i < 6; ++i) {
    int index = i + (shift ? 6 : 0);
    auto id = specs[index].id;
    attachments[i] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, id, faders[i]);
    faders[i].setTooltip(specs[index].name);
  }
  repaint();
}
void White224Editor::refreshPresets() {
  presets.clear(juce::dontSendNotification);
  juce::String bank;
  for (int i = 0; i < 42; ++i) {
    auto &p = factoryPresets()[static_cast<size_t>(i)];
    if (bank != juce::String(p.bank)) {
      bank = p.bank;
      presets.addSectionHeading(bank);
    }
    presets.addItem(p.name, i + 1);
  }
  userFiles.clear();
  auto dir = processor.userPresetDirectory();
  if (dir.isDirectory())
    dir.findChildFiles(userFiles, juce::File::findFiles, false, "*.w224");
  if (!userFiles.isEmpty()) {
    presets.addSectionHeading("User Presets");
    for (int i = 0; i < userFiles.size(); ++i)
      presets.addItem(userFiles[i].getFileNameWithoutExtension(), 100 + i);
  }
  presets.setText("FACTORY / USER PRESETS", juce::dontSendNotification);
}
void White224Editor::chooseFile(bool isSave) {
  chooser = std::make_unique<juce::FileChooser>(
      isSave ? "Save White224 Preset" : "Load White224 Preset",
      processor.userPresetDirectory(), "*.w224");
  auto safe = juce::Component::SafePointer<White224Editor>(this);
  chooser->launchAsync(
      (isSave ? juce::FileBrowserComponent::saveMode |
                    juce::FileBrowserComponent::warnAboutOverwriting
              : juce::FileBrowserComponent::openMode) |
          juce::FileBrowserComponent::canSelectFiles,
      [safe, isSave](const juce::FileChooser &c) {
        if (!safe)
          return;
        auto file = c.getResult();
        if (file == juce::File{})
          return;
        auto r =
            isSave ? safe->processor.savePreset(file.withFileExtension("w224"))
                   : safe->processor.loadPreset(file);
        if (r.failed())
          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::WarningIcon, "Preset",
              r.getErrorMessage());
        else
          safe->refreshPresets();
      });
}
void White224Editor::resized() {
  panel.setBounds(0, 0, 900, 780);
  panel.setTransform(
      juce::AffineTransform::scale(getWidth() / 900.f, getHeight() / 780.f));
  presets.setBounds(32, 18, 490, 27);
  save.setBounds(530, 18, 82, 27);
  load.setBounds(618, 18, 82, 27);
  design.setBounds(708, 18, 160, 27);
  for (int i = 0; i < 8; ++i) {
    programs[i].setBounds(48 + i * 101, 234, 95, 60);
    functions[i].setBounds(48 + i * 101, 306, 95, 42);
  }
  for (int i = 0; i < 6; ++i) {
    faders[i].setBounds(48 + i * 115, 424, 95, 229);
    values[i].setBounds(48 + i * 115, 674, 95, 36);
  }
  wet.setBounds(761, 542, 95, 65);
  bypass.setBounds(761, 640, 95, 65);
  persist();
}
void White224Editor::segmentText(juce::Graphics &g, juce::String text, float x,
                                 float y, float h) {
  constexpr int masks[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66,
                           0x6d, 0x7d, 0x07, 0x7f, 0x6f};
  float w = h * .53f, t = h * .075f;
  for (auto c : text) {
    if (c == '.') {
      g.setColour(red);
      g.fillEllipse(x, y + h - t, t, t);
      x += t * 2;
      continue;
    }
    int mask = (c >= '0' && c <= '9') ? masks[c - '0'] : c == '-' ? 0x40 : 0;
    std::array<juce::Rectangle<float>, 7> s = {
        juce::Rectangle<float>(x + t, y, w - 2 * t, t),
        {x + w - t, y + t, t, h * .5f - 1.5f * t},
        {x + w - t, y + h * .5f + t * .5f, t, h * .5f - 1.5f * t},
        {x + t, y + h - t, w - 2 * t, t},
        {x, y + h * .5f + t * .5f, t, h * .5f - 1.5f * t},
        {x, y + t, t, h * .5f - 1.5f * t},
        {x + t, y + h * .5f - t * .5f, w - 2 * t, t}};
    for (int k = 0; k < 7; ++k) {
      g.setColour(mask & (1 << k) ? red : juce::Colour(0xff401415));
      g.fillRoundedRectangle(s[k], t * .25f);
    }
    x += w + h * .12f;
  }
}
void White224Editor::drawDisplay(juce::Graphics &g) {
  g.setColour(juce::Colour(0xff171014));
  g.fillRoundedRectangle(48, 117, 804, 95, 3);
  g.setColour(juce::Colour(0xff5a4742));
  g.drawRoundedRectangle(48, 117, 804, 95, 3, 2);
  const auto s = processor.settings();
  float value = (s.values[Bass] + s.values[Mid]) * .5f;
  juce::String unit = "sec";
  if (held >= 0) {
    int i = held + (shift ? 6 : 0);
    value = s.values[i];
    unit = specs[i].unit;
    if (unit == "Hz" && value >= 1000) {
      value *= .001f;
      unit = "kHz";
    }
  }
  auto text = juce::String(value, value >= 100 ? 0 : 2);
  segmentText(g, text, 528, 137, 52);
  g.setFont(font(14));
  g.setColour(red);
  g.drawText(unit, 770, 168, 65, 24, juce::Justification::centred);
  g.setFont(font(11));
  g.setColour(juce::Colour(0xffc87c70));
  g.drawText("HEADROOM", 66, 125, 110, 17, juce::Justification::left);
  for (int c = 0; c < 2; ++c) {
    float level = c == 0 ? levelL : levelR;
    g.setColour(red);
    g.drawText(c == 0 ? "L" : "R", 67, 146 + c * 21, 20, 18,
               juce::Justification::left);
    for (int j = 0; j < 5; ++j) {
      bool on = level >= std::pow(10.f, (-24.f + 6 * j) / 20.f);
      g.setColour(on ? red : juce::Colour(0xff4a2022));
      g.fillRect(96 + j * 36, 151 + c * 21, 25, 8);
    }
  }
  g.setFont(font(10));
  g.setColour(juce::Colour(0xffbc7067));
  for (int j = 0; j < 5; ++j)
    g.drawText(juce::String(24 - j * 6), 94 + j * 36, 187, 30, 15,
               juce::Justification::centred);
  g.drawText("dB", 280, 187, 30, 15, juce::Justification::left);
  led(g, 324, 153, levelL > 1 || levelR > 1);
  g.drawText("ovfl", 314, 174, 40, 15, juce::Justification::centred);
}
void White224Editor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff151919));
  g.addTransform(
      juce::AffineTransform::scale(getWidth() / 900.f, getHeight() / 780.f));
  if (look.realistic)
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff2eee1), 0, 50,
                                           juce::Colour(0xffd0cbbd), 850, 780,
                                           false));
  else
    g.setColour(cream);
  g.fillRoundedRectangle(14, 7, 872, 766, 12);
  g.setColour(ink);
  g.setFont(font(17, true));
  g.drawText("WHITE224", 48, 61, 260, 22, juce::Justification::left);
  g.setFont(font(10, true));
  g.drawText("VINTAGE DIGITAL REVERBERATION", 49, 84, 350, 14,
             juce::Justification::left);
  g.setFont(font(27));
  g.drawText("224 White Reverb", 500, 64, 351, 34, juce::Justification::right);
  drawDisplay(g);
  const auto s = processor.settings();
  g.setFont(font(12, true));
  g.setColour(ink);
  g.drawText(juce::String(s.program + 1) + " / " +
                 ::white224::programs()[s.program].name,
             50, 360, 660, 21, juce::Justification::left);
  g.setFont(font(10));
  g.drawText(shift ? "SHIFT / SECONDARY PARAMETERS"
                   : "REVERB / PRIMARY PARAMETERS",
             50, 385, 650, 17, juce::Justification::left);
  for (int i = 0; i < 6; ++i) {
    int index = i + (shift ? 6 : 0);
    g.setFont(font(11, true));
    g.drawFittedText(specs[index].name, 42 + i * 115, 402, 108, 23,
                     juce::Justification::centred, 2);
    g.setFont(font(10));
    juce::String label =
        juce::String(s.values[index], index == Crossover || index == Treble ||
                                              index == Bandwidth ||
                                              index == Enhancement ||
                                              index == Optimization
                                          ? 0
                                          : 2) +
        " " + specs[index].unit;
    g.drawText(label, 42 + i * 115, 650, 108, 20, juce::Justification::centred);
  }
  g.setColour(juce::Colour(0xff8c887f));
  g.drawLine(741, 400, 741, 717);
  g.setFont(font(10, true));
  g.setColour(ink);
  g.drawFittedText("MODE\nENHANCEMENT", 752, 414, 115, 32,
                   juce::Justification::centred, 2);
  led(g, 804, 453, s.modeEnabled);
  g.setColour(ink);
  g.drawFittedText("DECAY\nOPTIMIZATION", 752, 476, 115, 32,
                   juce::Justification::centred, 2);
  led(g, 804, 515, s.decayEnabled);
  g.setFont(font(10));
  g.setColour(juce::Colour(0xff69675f));
  g.drawText(
      "SHIFT + 1  CHORUS     /     SHIFT + 7  MODE     /     SHIFT + 8  DECAY",
      48, 728, 705, 22, juce::Justification::left);
  g.drawText("WHITE224  /  0.1", 754, 728, 104, 22, juce::Justification::right);
  if (look.realistic) {
    for (auto point :
         {juce::Point<float>(29, 65), {871, 65}, {29, 752}, {871, 752}}) {
      g.setColour(juce::Colour(0xffa19f95));
      g.fillEllipse(point.x - 4, point.y - 4, 8, 8);
      g.setColour(juce::Colour(0xff55564f));
      g.drawLine(point.x - 2, point.y - 2, point.x + 2, point.y + 2);
    }
  }
}
void White224Editor::timerCallback() {
  const auto s = processor.settings();
  levelL = std::max(processor.meterL.load(), levelL * .87f);
  levelR = std::max(processor.meterR.load(), levelR * .87f);
  for (int i = 0; i < 8; ++i)
    programs[i].setToggleState(
        shift && i == 6   ? s.modeEnabled
        : shift && i == 7 ? s.decayEnabled
                          : s.program == i || (s.program == 8 && i == 0),
        juce::dontSendNotification);
  functions[0].setToggleState(!call, juce::dontSendNotification);
  functions[1].setToggleState(armed, juce::dontSendNotification);
  functions[2].setToggleState(call, juce::dontSendNotification);
  functions[3].setToggleState(shift, juce::dontSendNotification);
  for (int i = 0; i < 4; ++i)
    functions[i + 4].setToggleState(processor.hasRegister(i) ||
                                        (regFlash == i && flashTicks > 0),
                                    juce::dontSendNotification);
  if (flashTicks > 0)
    --flashTicks;
  for (int i = 0; i < 6; ++i) {
    const int index = i + (shift ? 6 : 0);
    faders[i].setDoubleClickReturnValue(true,
                                        recommended(s.program).values[index]);
    values[i].setToggleState(held == i, juce::dontSendNotification);
  }
  const int factory = processor.getCurrentProgram();
  if (factory != lastFactory) {
    presets.setSelectedId(factory + 1, juce::dontSendNotification);
    lastFactory = factory;
  }
  design.setButtonText(look.realistic ? "3D LOOK" : "CLASSIC LOOK");
  repaint();
}
