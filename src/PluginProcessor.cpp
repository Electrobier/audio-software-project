#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <charconv>
using namespace white224;
juce::AudioProcessorValueTreeState::ParameterLayout
White224Processor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout p;
  for (const auto &s : specs) {
    juce::NormalisableRange<float> r;
    if (s.logarithmic) {
      const float lo = s.min, hi = s.max;
      r = {
          lo, hi,
          [lo, hi](float, float, float n) { return lo * std::pow(hi / lo, n); },
          [lo, hi](float, float, float v) {
            return std::log(v / lo) / std::log(hi / lo);
          }};
    } else
      r = {s.min, s.max,
           (juce::String(s.id) == "enhancement" ||
            juce::String(s.id) == "optimization")
               ? 1.f
               : 0.f};
    p.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{s.id, 1}, s.name, r, s.initial,
        juce::AudioParameterFloatAttributes().withLabel(s.unit)));
  }
  juce::StringArray names;
  for (auto &a : programs())
    names.add(a.name);
  p.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"algorithm", 1}, "Algorithm", names, 0));
  for (auto pair :
       {std::pair{"modeEnabled", "Mode Enhancement enabled"},
        std::pair{"decayEnabled", "Decay Optimization enabled"},
        std::pair{"wet", "100% Wet"}, std::pair{"bypass", "Bypass"}})
    p.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{pair.first, 1}, pair.second,
        juce::String(pair.first).contains("Enabled")));
  return p;
}
White224Processor::White224Processor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "PARAMETERS", layout()) {
  for (int i = 0; i < Count; ++i)
    raw[i] = state.getRawParameterValue(specs[i].id);
  algorithm = state.getRawParameterValue("algorithm");
  mode = state.getRawParameterValue("modeEnabled");
  decay = state.getRawParameterValue("decayEnabled");
  wet = state.getRawParameterValue("wet");
  bypass = state.getRawParameterValue("bypass");
}
Settings White224Processor::settings() const {
  Settings s;
  for (int i = 0; i < Count; ++i)
    s.values[i] = raw[i]->load();
  s.program = static_cast<int>(algorithm->load());
  s.modeEnabled = mode->load() > .5f;
  s.decayEnabled = decay->load() > .5f;
  s.wet = wet->load() > .5f;
  s.bypass = bypass->load() > .5f;
  return s;
}
void White224Processor::prepareToPlay(double sr, int) {
  engine.setSettings(settings());
  engine.prepare(sr);
}
bool White224Processor::isBusesLayoutSupported(const BusesLayout &b) const {
  return b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
         (b.getMainInputChannelSet() == juce::AudioChannelSet::mono() ||
          b.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}
void White224Processor::render(juce::AudioBuffer<float> &b, bool hostBypass) {
  juce::ScopedNoDenormals noDenormals;
  if (b.getNumChannels() < 2)
    return;
  if (getTotalNumInputChannels() == 1)
    b.copyFrom(1, 0, b, 0, 0, b.getNumSamples());
  auto s = settings();
  s.bypass = s.bypass || hostBypass;
  engine.setSettings(s);
  engine.process(b.getWritePointer(0), b.getWritePointer(1), b.getNumSamples());
  meterL.store(engine.peakLeft);
  meterR.store(engine.peakRight);
}
void White224Processor::processBlock(juce::AudioBuffer<float> &b,
                                     juce::MidiBuffer &) {
  render(b, false);
}
void White224Processor::processBlockBypassed(juce::AudioBuffer<float> &b,
                                             juce::MidiBuffer &) {
  render(b, true);
}
juce::AudioProcessorParameter *White224Processor::getBypassParameter() const {
  return state.getParameter("bypass");
}
void White224Processor::setParameter(const juce::String &id, float v) {
  if (auto *p = state.getParameter(id)) {
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(v));
    p->endChangeGesture();
  }
}
void White224Processor::apply(const Settings &in) {
  auto s = in;
  s.sanitize();
  for (int i = 0; i < Count; ++i)
    setParameter(specs[i].id, s.values[i]);
  setParameter("algorithm", static_cast<float>(s.program));
  setParameter("modeEnabled", s.modeEnabled ? 1.f : 0.f);
  setParameter("decayEnabled", s.decayEnabled ? 1.f : 0.f);
  setParameter("wet", s.wet ? 1.f : 0.f);
  setParameter("bypass", s.bypass ? 1.f : 0.f);
}
void White224Processor::selectAlgorithm(int p, bool call) {
  p = juce::jlimit(0, 8, p);
  if (call) {
    auto s = settings(), r = recommended(p);
    for (int i = 0; i < 6; ++i)
      s.values[i] = r.values[i];
    s.program = p;
    apply(s);
  } else
    setParameter("algorithm", static_cast<float>(p));
}
void White224Processor::setCurrentProgram(int i) {
  if (i >= 0 && i < 42) {
    apply(factoryPresets()[static_cast<size_t>(i)].settings);
    presetIndex = i;
  }
}
const juce::String White224Processor::getProgramName(int i) {
  return i >= 0 && i < 42 ? factoryPresets()[static_cast<size_t>(i)].name
                          : juce::String{};
}
void White224Processor::storeRegister(int i) {
  if (i < 0 || i > 3)
    return;
  auto s = state.copyState();
  const juce::ScopedLock lock(metadataLock);
  registers[static_cast<size_t>(i)] = s;
}
bool White224Processor::recallRegister(int i) {
  if (i < 0 || i > 3)
    return false;
  juce::ValueTree v;
  {
    const juce::ScopedLock lock(metadataLock);
    v = registers[static_cast<size_t>(i)].createCopy();
  }
  if (!v.isValid())
    return false;
  state.replaceState(v);
  return true;
}
bool White224Processor::hasRegister(int i) const {
  const juce::ScopedLock lock(metadataLock);
  return i >= 0 && i < 4 && registers[static_cast<size_t>(i)].isValid();
}
juce::ValueTree White224Processor::snapshot() {
  juce::ValueTree root("White224");
  root.setProperty("version", 1, nullptr);
  root.setProperty("factoryIndex", presetIndex.load(), nullptr);
  root.addChild(state.copyState(), -1, nullptr);
  const juce::ScopedLock lock(metadataLock);
  root.addChild(ui.createCopy(), -1, nullptr);
  juce::ValueTree regs("Registers");
  for (int i = 0; i < 4; ++i) {
    juce::ValueTree slot("Slot");
    slot.setProperty("index", i, nullptr);
    if (registers[static_cast<size_t>(i)].isValid())
      slot.addChild(registers[static_cast<size_t>(i)].createCopy(), -1,
                    nullptr);
    regs.addChild(slot, -1, nullptr);
  }
  root.addChild(regs, -1, nullptr);
  return root;
}
bool White224Processor::validParameters(const juce::ValueTree &v) {
  if (!v.hasType("PARAMETERS") || v.getNumChildren() != Count + 5)
    return false;
  juce::StringArray seen;
  for (auto child : v) {
    if (!child.hasType("PARAM") || !child.hasProperty("id") ||
        !child.hasProperty("value"))
      return false;
    const auto id = child["id"].toString();
    if (seen.contains(id))
      return false;
    seen.add(id);
    double lo = 0, hi = 1;
    bool found = false;
    for (auto &s : specs)
      if (id == s.id) {
        lo = s.min;
        hi = s.max;
        found = true;
        break;
      }
    if (id == "algorithm") {
      lo = 0;
      hi = 8;
      found = true;
    }
    for (auto b : {"modeEnabled", "decayEnabled", "wet", "bypass"})
      if (id == b)
        found = true;
    auto value = child["value"];
    double x = 0;
    if (value.isString()) {
      auto text = value.toString().toStdString();
      auto parsed = std::from_chars(text.data(), text.data() + text.size(), x);
      if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        return false;
    } else if (value.isDouble() || value.isInt() || value.isInt64() ||
               value.isBool())
      x = static_cast<double>(value);
    else
      return false;
    if (!found || !std::isfinite(x) || x < lo || x > hi)
      return false;
    if ((id == "algorithm" || id == "modeEnabled" || id == "decayEnabled" ||
         id == "wet" || id == "bypass" || id == "enhancement" ||
         id == "optimization") &&
        x != std::floor(x))
      return false;
  }
  return true;
}
bool White224Processor::restore(const juce::ValueTree &root) {
  if (!root.hasType("White224") || static_cast<int>(root["version"]) != 1)
    return false;
  auto params = root.getChildWithName("PARAMETERS");
  if (!validParameters(params))
    return false;
  std::array<juce::ValueTree, 4> regs;
  auto rs = root.getChildWithName("Registers");
  std::array<bool, 4> seen{};
  for (auto slot : rs) {
    int i = slot.hasProperty("index") ? static_cast<int>(slot["index"]) : -1;
    if (i < 0 || i > 3 || seen[static_cast<size_t>(i)] ||
        slot.getNumChildren() > 1)
      return false;
    seen[static_cast<size_t>(i)] = true;
    if (slot.getNumChildren()) {
      if (!validParameters(slot.getChild(0)))
        return false;
      regs[static_cast<size_t>(i)] = slot.getChild(0).createCopy();
    }
  }
  state.replaceState(params.createCopy());
  {
    const juce::ScopedLock lock(metadataLock);
    registers = regs;
    auto newUi = root.getChildWithName("UI");
    ui = newUi.isValid() ? newUi.createCopy() : juce::ValueTree("UI");
  }
  presetIndex = juce::jlimit(
      0, 41, static_cast<int>(root.getProperty("factoryIndex", 0)));
  return true;
}
void White224Processor::getStateInformation(juce::MemoryBlock &b) {
  auto xml = snapshot().createXml();
  copyXmlToBinary(*xml, b);
}
void White224Processor::setStateInformation(const void *d, int n) {
  if (n < 1 || n > 1024 * 1024)
    return;
  auto xml = getXmlFromBinary(d, n);
  if (xml)
    restore(juce::ValueTree::fromXml(*xml));
}
juce::File White224Processor::userPresetDirectory() const {
  return juce::File::getSpecialLocation(
             juce::File::userApplicationDataDirectory)
      .getChildFile("White224/Presets");
}
juce::Result White224Processor::savePreset(const juce::File &f) {
  auto parent = f.getParentDirectory();
  auto r = parent.createDirectory();
  if (r.failed())
    return r;
  juce::TemporaryFile temp(f);
  if (!temp.getFile().replaceWithText(snapshot().toXmlString()) ||
      !temp.overwriteTargetFileWithTemporary())
    return juce::Result::fail("Preset konnte nicht atomar gespeichert werden.");
  return juce::Result::ok();
}
juce::Result White224Processor::loadPreset(const juce::File &f) {
  if (!f.existsAsFile() || f.getSize() > 1024 * 1024)
    return juce::Result::fail("Datei fehlt oder ist zu groß.");
  auto xml = juce::XmlDocument::parse(f);
  if (!xml || !restore(juce::ValueTree::fromXml(*xml)))
    return juce::Result::fail(
        "Ungültiges Preset oder nicht unterstützte Version.");
  return juce::Result::ok();
}
void White224Processor::setUi(int w, int h, bool realistic, bool shift,
                              bool call) {
  const juce::ScopedLock lock(metadataLock);
  ui.setProperty("width", w, nullptr);
  ui.setProperty("height", h, nullptr);
  ui.setProperty("realistic", realistic, nullptr);
  ui.setProperty("shift", shift, nullptr);
  ui.setProperty("call", call, nullptr);
}
std::array<int, 2> White224Processor::uiSize() const {
  const juce::ScopedLock lock(metadataLock);
  return {
      juce::jlimit(630, 1440, static_cast<int>(ui.getProperty("width", 900))),
      juce::jlimit(546, 1248, static_cast<int>(ui.getProperty("height", 780)))};
}
bool White224Processor::uiFlag(const juce::String &id, bool fallback) const {
  const juce::ScopedLock lock(metadataLock);
  return static_cast<bool>(ui.getProperty(id, fallback));
}
juce::AudioProcessorEditor *White224Processor::createEditor() {
  return new White224Editor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new White224Processor;
}
