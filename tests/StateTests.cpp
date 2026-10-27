#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
void check(bool b, const char *msg) {
  if (!b)
    throw std::runtime_error(msg);
}
bool same(const std::array<float, white224::Count> &a,
          const std::array<float, white224::Count> &b) {
  for (int i = 0; i < white224::Count; ++i) {
    if (std::abs(a[i] - b[i]) > std::max(1.e-5f, std::abs(a[i]) * 2.e-6f)) {
      std::cerr << "Parameter " << i << " actual=" << a[i]
                << " expected=" << b[i] << "\n";
      return false;
    }
  }
  return true;
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI init;
  try {
    White224Processor p;
    p.setCurrentProgram(18);
    p.setParameter("wet", 1);
    p.storeRegister(0);
    auto expected = p.settings();
    p.setCurrentProgram(2);
    check(p.recallRegister(0), "register unavailable");
    check(same(p.settings().values, expected.values), "register values");
    juce::MemoryBlock block;
    p.getStateInformation(block);
    White224Processor q;
    q.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
    check(same(q.settings().values, p.settings().values), "state mismatch");
    check(q.settings().program == p.settings().program && q.settings().wet,
          "state flags");
    check(q.hasRegister(0), "register not persisted");
    q.setCurrentProgram(4);
    q.recallRegister(0);
    check(same(q.settings().values, expected.values), "register after reload");
    const auto before = q.settings();
    const char garbage[] = "broken state";
    q.setStateInformation(garbage, sizeof(garbage));
    check(q.settings().values == before.values, "corrupt state changed values");
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getNonexistentChildFile("White224-validation", "", false);
    dir.createDirectory();
    auto file = dir.getChildFile("test.w224");
    check(p.savePreset(file).wasOk(), "save failed");
    check(q.loadPreset(file).wasOk(), "load failed");
    check(same(q.settings().values, p.settings().values),
          "file state mismatch");
    auto xml = juce::XmlDocument::parse(file);
    check(xml != nullptr, "xml missing");
    auto *params = xml->getChildByName("PARAMETERS");
    check(params != nullptr, "params missing");
    auto validXml = std::make_unique<juce::XmlElement>(*xml);
    params->getChildElement(0)->setAttribute("value", "nan");
    file.replaceWithText(xml->toString());
    auto original = q.settings();
    check(q.loadPreset(file).failed(), "NaN preset accepted");
    check(q.settings().values == original.values,
          "invalid preset partially applied");

    auto reject = [&](std::unique_ptr<juce::XmlElement> candidate,
                      const char *why) {
      file.replaceWithText(candidate->toString());
      check(q.loadPreset(file).failed(), why);
      check(q.settings().values == original.values,
            "bad file partially applied");
    };
    auto unknown = std::make_unique<juce::XmlElement>(*validXml);
    unknown->getChildByName("PARAMETERS")
        ->getChildElement(0)
        ->setAttribute("id", "unknown");
    reject(std::move(unknown), "unknown parameter accepted");
    auto range = std::make_unique<juce::XmlElement>(*validXml);
    range->getChildByName("PARAMETERS")
        ->getChildElement(0)
        ->setAttribute("value", 1e9);
    reject(std::move(range), "out of range accepted");
    auto version = std::make_unique<juce::XmlElement>(*validXml);
    version->setAttribute("version", 99);
    reject(std::move(version), "future version accepted");
    auto duplicate = std::make_unique<juce::XmlElement>(*validXml);
    auto *nodes = duplicate->getChildByName("PARAMETERS");
    nodes->addChildElement(new juce::XmlElement(*nodes->getChildElement(0)));
    reject(std::move(duplicate), "duplicate parameter accepted");
    file.replaceWithText("<White224><broken");
    check(q.loadPreset(file).failed(), "broken XML accepted");
    check(q.settings().values == original.values,
          "broken XML partially applied");
    dir.deleteRecursively();
    for (int i = 0; i < 42; ++i) {
      q.setCurrentProgram(i);
      check(q.settings().program ==
                white224::factoryPresets()[static_cast<size_t>(i)]
                    .settings.program,
            "preset program");
    }
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add(juce::AudioChannelSet::mono());
    mono.outputBuses.add(juce::AudioChannelSet::stereo());
    check(q.setBusesLayout(mono), "mono layout rejected");
    q.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> audio(2, 256);
    audio.clear();
    audio.setSample(0, 0, .5f);
    juce::MidiBuffer midi;
    q.processBlock(audio, midi);
    check(std::isfinite(audio.getSample(1, 0)), "mono render");

    q.apply(white224::recommended(0));
    std::unique_ptr<juce::AudioProcessorEditor> editor(q.createEditor());
    auto *panel = editor->getChildComponent(0);
    check(panel != nullptr, "panel missing");
    auto button = [&](const juce::String &text) -> juce::TextButton * {
      for (auto *child : panel->getChildren())
        if (auto *b = dynamic_cast<juce::TextButton *>(child))
          if (b->getButtonText() == text)
            return b;
      throw std::runtime_error("button missing");
    };
    auto click = [&](const juce::String &text) {
      auto *b = button(text);
      check(bool(b->onClick), "button callback missing");
      b->onClick();
    };
    for (int i = 0; i < 8; ++i) {
      click(juce::String(i + 1));
      check(q.settings().program == i, "GUI program selection");
      check(same(q.settings().values, white224::recommended(i).values),
            "GUI CALL recommended values");
    }
    click("IMMED");
    q.setParameter("bass", 12);
    const auto immediate = q.settings();
    click("3");
    check(q.settings().values == immediate.values, "GUI IMMED changed faders");
    click("SET");
    click("D");
    q.setParameter("bass", 2);
    click("D");
    check(std::abs(q.settings().values[white224::Bass] - 12) < .001f,
          "GUI register D");
    click("SHIFT");
    click("1");
    check(q.settings().program == 8, "GUI SHIFT chorus");
    auto wasMode = q.settings().modeEnabled;
    click("7");
    check(q.settings().modeEnabled != wasMode, "GUI mode toggle");
    auto wasDecay = q.settings().decayEnabled;
    click("8");
    check(q.settings().decayEnabled != wasDecay, "GUI decay toggle");
    int slider = 0;
    for (auto *child : panel->getChildren())
      if (auto *f = dynamic_cast<juce::Slider *>(child)) {
        if (slider == 0) {
          f->setValue(58, juce::sendNotificationSync);
          check(std::abs(q.settings().values[white224::Mix] - 58) < .001f,
                "GUI SHIFT fader attachment");
        }
        ++slider;
      }
    check(slider == 6, "GUI six faders");
    button("100% WET")->setToggleState(true, juce::sendNotificationSync);
    check(q.settings().wet, "GUI wet attachment");
    button("BYPASS")->setToggleState(true, juce::sendNotificationSync);
    check(q.settings().bypass, "GUI bypass attachment");
    click("SHIFT");
    click("CALL");
    q.apply(white224::recommended(0));
    editor->setSize(630, 546);
    check(editor->getWidth() == 630, "GUI minimum resize");
    editor->setSize(1440, 1248);
    check(editor->getWidth() == 1440, "GUI maximum resize");
    editor->setSize(900, 780);
    // Recreate to initialize feedback LEDs synchronously for reproducible
    // previews.
    editor.reset();
    editor.reset(q.createEditor());
    if (argc > 1) {
      auto capture = [&](const juce::File &picture) {
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        auto stream = picture.createOutputStream();
        check(stream != nullptr, "screenshot file");
        juce::PNGImageFormat png;
        check(png.writeImageToStream(image, *stream), "screenshot encode");
      };
      juce::File picture(juce::String::fromUTF8(argv[1]));
      capture(picture);
      panel = editor->getChildComponent(0);
      const auto stateBefore = q.settings();
      click("3D LOOK");
      capture(picture.getSiblingFile("Classic.png"));
      check(q.settings().values == stateBefore.values, "design changed audio");
      click("3D LOOK");
    }
    std::cout << "PASS: APVTS state, registers, factory presets, disk presets, "
                 "corrupted state, mono/stereo layout, optional GUI snapshot\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
