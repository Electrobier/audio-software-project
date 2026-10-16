// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 IRON 2500 contributors
#include "PluginEditor.h"
using namespace juce;
IronLook::IronLook()
{
    setColour(Slider::textBoxTextColourId, Colour(0xffe1e2dc));
    setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour(Slider::textBoxBackgroundColourId, Colour(0xff151a1f));
    setColour(ComboBox::backgroundColourId, Colour(0xff161e26));
    setColour(ComboBox::outlineColourId, Colour(0xff55616a));
    setColour(TextButton::buttonColourId, Colour(0xff28333d));
    setColour(ToggleButton::tickColourId, Colour(0xffa8d97e));
}
void IronLook::drawRotarySlider(Graphics &g, int x, int y, int w, int h, float p, float start, float end,
                                Slider &s)
{
    auto box = Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(13);
    auto r = jmin(box.getWidth(), box.getHeight()) * .5f;
    auto c = box.getCentre();
    for (int i = 0; i <= 10; ++i)
    {
        float a = start + (end - start) * i / 10;
        Point<float> v{std::sin(a), -std::cos(a)};
        g.setColour(Colour(0xff939a9c));
        g.drawLine({c + v * (r + 5), c + v * (r + 9)}, i % 5 == 0 ? 2.f : 1.f);
    }
    auto circle = Rectangle<float>(r * 2, r * 2).withCentre(c);
    g.setColour(Colours::black.withAlpha(.65f));
    g.fillEllipse(circle.translated(2, 5).expanded(2));
    bool red = s.getProperties().getWithDefault("red", false);
    Colour base = red ? Colour(0xffa7352c) : Colour(0xff30353a);
    g.setGradientFill(
        ColourGradient(base.brighter(.3f), c.x, c.y - r, base.darker(.8f), c.x, c.y + r, false));
    g.fillEllipse(circle);
    g.setColour(Colour(0xff06080a));
    g.drawEllipse(circle, 2);
    for (int i = 0; i < 36; ++i)
    {
        float a = 2 * MathConstants<float>::pi * i / 36;
        Point<float> v{std::sin(a), std::cos(a)};
        g.setColour(Colours::black.withAlpha(.38f));
        g.drawLine({c + v * (r - 5), c + v * (r - 1)}, 1.3f);
    }
    g.setColour(base.darker(.3f));
    g.fillEllipse(circle.reduced(6));
    g.setColour(Colours::white.withAlpha(.12f));
    g.drawEllipse(circle.reduced(6), 1);
    float a = start + (end - start) * p;
    Point<float> v{std::sin(a), -std::cos(a)};
    g.setColour(Colour(0xffeee8d7));
    g.drawLine({c + v * (r * .35f), c + v * (r * .78f)}, 3);
}
IronEditor::IronEditor(IronProcessor &p) : AudioProcessorEditor(p), processor(p), face(*this)
{
    setLookAndFeel(&look);
    addAndMakeVisible(face);
    setResizable(true, true);
    setResizeLimits(900, 495, 2400, 1320);
    getConstrainer()->setFixedAspectRatio(1200. / 660.);
    setSize(1200, 660);
    knob("threshold", "THRESHOLD", {43, 287, 155, 151});
    knob("attack", "ATTACK / ms", {224, 287, 155, 151});
    knob("ratio", "RATIO", {405, 287, 155, 151});
    knob("release", "RELEASE / s", {586, 287, 155, 151});
    knob("variable", "VARIABLE / %", {767, 287, 155, 151});
    choice("knee", "KNEE", {57, 521, 144, 34});
    choice("thrust", "THRUST", {230, 521, 144, 34});
    choice("type", "TYPE", {403, 521, 170, 34});
    choice("shape", "SHAPE", {628, 521, 125, 34});
    knob("link", "L/R LINK / %", {792, 477, 130, 128});
    knob("output", "OUTPUT / dB", {1012, 242, 145, 132}, true);
    knob("mix", "MIX / %", {1012, 414, 145, 106});
    knob("trim", "TRIM / dB", {1012, 552, 145, 90});
    choice("meter", "VU SOURCE", {690, 129, 180, 35});
    face.addAndMakeVisible(analog);
    analog.setBounds(1034, 143, 110, 32);
    analog.setTooltip("2x oversampled, symmetric analog saturation; no added noise.");
    face.addAndMakeVisible(bypass);
    bypass.setBounds(1034, 180, 110, 32);
    bypass.setTooltip("Latency-compensated dry bypass.");
    analogAttachment =
        std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.state, "analog", analog);
    bypassAttachment =
        std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.state, "bypass", bypass);
    for (auto *b : {&previous, &next, &ab, &menu})
        face.addAndMakeVisible(*b);
    face.addAndMakeVisible(preset);
    previous.setBounds(31, 19, 28, 26);
    preset.setBounds(65, 19, 255, 26);
    next.setBounds(326, 19, 28, 26);
    ab.setBounds(793, 19, 67, 26);
    menu.setBounds(870, 19, 100, 26);
    for (int i = 0; i < p.getNumPrograms(); ++i)
        preset.addItem(p.getProgramName(i), i + 1);
    preset.setSelectedId(p.getCurrentProgram() + 1, dontSendNotification);
    preset.onChange = [this] { processor.setCurrentProgram(preset.getSelectedId() - 1); };
    previous.onClick = [this] { preset.setSelectedId((preset.getSelectedId() + 10) % 12 + 1); };
    next.onClick = [this] { preset.setSelectedId(preset.getSelectedId() % 12 + 1); };
    ab.onClick = [this] { processor.swapAB(); };
    ab.setTooltip("Swap current settings with the alternate snapshot. Both begin at Clean.");
    menu.onClick = [this] { showMenu(); };
    startTimerHz(30);
}
IronEditor::~IronEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}
void IronEditor::label(const String &text, Rectangle<int> b)
{
    auto l = std::make_unique<Label>();
    l->setText(text, dontSendNotification);
    l->setFont(FontOptions(12.f, Font::bold));
    l->setColour(Label::textColourId, Colour(0xffd8dcda));
    l->setJustificationType(Justification::centred);
    l->setBounds(b);
    face.addAndMakeVisible(*l);
    labels.push_back(std::move(l));
}
void IronEditor::knob(const char *id, const char *title, Rectangle<int> b, bool red)
{
    auto s = std::make_unique<Slider>();
    s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle(Slider::TextBoxBelow, false, 108, 21);
    s->setRotaryParameters(MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
    s->setPopupMenuEnabled(true);
    s->setVelocityModeParameters(1., 1, .1, true, ModifierKeys::shiftModifier);
    s->getProperties().set("red", red);
    auto *p = processor.state.getParameter(id);
    s->setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    s->setTooltip(p->getName(64) +
                  " / double click: default / Shift drag: fine / value field: numeric input");
    s->setBounds(b);
    face.addAndMakeVisible(*s);
    label(title, {b.getX() - 10, b.getY() - 25, b.getWidth() + 20, 22});
    sliderAttachments.push_back(
        std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(processor.state, id, *s));
    knobs.push_back(std::move(s));
}
void IronEditor::choice(const char *id, const char *title, Rectangle<int> b)
{
    auto c = std::make_unique<ComboBox>();
    auto *p = dynamic_cast<AudioParameterChoice *>(processor.state.getParameter(id));
    if (p)
        c->addItemList(p->choices, 1);
    c->setBounds(b);
    c->setTooltip(title);
    face.addAndMakeVisible(*c);
    label(title, {b.getX(), b.getY() - 25, b.getWidth(), 22});
    choiceAttachments.push_back(
        std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state, id, *c));
    choices.push_back(std::move(c));
}
void IronEditor::resized()
{
    face.setBounds(0, 0, 1200, 660);
    face.setTransform(AffineTransform::scale(getWidth() / 1200.f));
}
void IronEditor::timerCallback()
{
    int mode = (int)processor.state.getRawParameterValue("meter")->load();
    for (int ch = 0; ch < 2; ++ch)
    {
        float v = processor.meters[(size_t)(mode * 2 + ch)].load(std::memory_order_relaxed);
        float target = mode == 0 ? 3 - v * 23 / 20 : v + 18;
        target = jlimit(-20.f, 3.f, target);
        needles[(size_t)ch] += .22f * (target - needles[(size_t)ch]);
    }
    face.repaint(35, 72, 610, 166);
}
void IronEditor::drawMeter(Graphics &g, Rectangle<float> b, float value, int channel)
{
    g.setColour(Colour(0xff050708));
    g.fillRoundedRectangle(b.expanded(5), 6);
    g.setGradientFill(ColourGradient(Colour(0xffffe8ac), b.getCentreX(), b.getY(), Colour(0xffa88c53),
                                     b.getCentreX(), b.getBottom(), false));
    g.fillRoundedRectangle(b, 3);
    auto pivot = Point<float>(b.getCentreX(), b.getBottom() + 20);
    float radius = b.getHeight() * .97f;
    for (int i = 0; i <= 10; ++i)
    {
        float a = -1.04f + i * .208f;
        Point<float> v{std::sin(a), -std::cos(a)};
        g.setColour(i > 7 ? Colour(0xff9d3025) : Colour(0xff303029));
        g.drawLine({pivot + v * radius, pivot + v * (radius - 8)}, 1.4f);
        if (i % 2 == 0)
        {
            auto at = pivot + v * (radius - 20);
            g.setFont(10.f);
            g.drawText(((int)processor.state.getRawParameterValue("meter")->load() == 0
                            ? String(20 - i * 2)
                            : (i == 10 ? String("+3") : String(-20 + i * 2))),
                       Rectangle<float>(28, 14).withCentre(at), Justification::centred);
        }
    }
    g.setColour(Colour(0xff3a3326));
    g.setFont(FontOptions(18.f, Font::bold));
    g.drawText("VU", b.withY(b.getY() + 65).withHeight(25), Justification::centred);
    g.setFont(9.f);
    g.drawText(channel == 0 ? "L     IRON INSTRUMENTS" : "R     IRON INSTRUMENTS",
               b.withY(b.getY() + 99).withHeight(16), Justification::centred);
    float a = -1.04f + (value + 20) / 23 * 2.08f;
    Point<float> tip = pivot + Point<float>(std::sin(a), -std::cos(a)) * (radius - 3);
    Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(b.toNearestInt());
    g.setColour(Colours::black.withAlpha(.25f));
    g.drawLine({pivot.translated(2, 2), tip.translated(2, 2)}, 3);
    g.setColour(Colour(0xff7a251e));
    g.drawLine({pivot, tip}, 1.8f);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(.18f), b.getX(), b.getY(),
                                     Colours::transparentWhite, b.getRight(), b.getBottom(), false));
    g.fillRect(b);
}
void IronEditor::Face::paint(Graphics &g)
{
    g.fillAll(Colour(0xff15191d));
    g.setGradientFill(ColourGradient(Colour(0xff30373c), 0, 0, Colour(0xff101417), 0, 660, false));
    g.fillRoundedRectangle(7, 58, 1186, 595, 8);
    g.setColour(Colour(0xff20262b));
    g.fillRoundedRectangle(24, 69, 946, 180, 5);
    g.setColour(Colour(0xff434b50));
    g.drawRoundedRectangle(24, 69, 946, 180, 5, 1);
    g.setGradientFill(ColourGradient(Colour(0xff28617f), 994, 60, Colour(0xff102c43), 1192, 650, false));
    g.fillRoundedRectangle(991, 60, 201, 590, 5);
    for (int y = 62; y < 650; y += 3)
    {
        g.setColour(Colours::white.withAlpha(y % 9 == 0 ? .025f : .008f));
        g.drawHorizontalLine(y, 12, 1188);
    }
    for (auto pt :
         {Point<float>(19, 70), Point<float>(1180, 70), Point<float>(19, 640), Point<float>(1180, 640)})
    {
        g.setColour(Colour(0xff0a0d0f));
        g.fillEllipse(pt.x - 6, pt.y - 6, 12, 12);
        g.setColour(Colour(0xff60656a));
        g.drawEllipse(pt.x - 5, pt.y - 5, 10, 10, 1);
        g.drawLine(pt.x - 3, pt.y + 2, pt.x + 3, pt.y - 2, 1);
    }
    owner.drawMeter(g, {47, 88, 265, 132}, owner.needles[0], 0);
    owner.drawMeter(g, {340, 88, 265, 132}, owner.needles[1], 1);
    g.setColour(Colour(0xffe2e1d7));
    g.setFont(FontOptions(26.f, Font::bold));
    g.drawText("IRON 2500", 997, 85, 188, 35, Justification::centred);
    g.setFont(10.f);
    g.drawText("STEREO BUS COMPRESSOR", 995, 120, 190, 20, Justification::centred);
    g.setColour(Colour(0xff77838a));
    g.setFont(11.f);
    g.drawText("VCA DYNAMICS  /  STEREO PRECISION", 645, 192, 302, 25, Justification::centred);
    g.drawText("IRON INSTRUMENTS", 460, 21, 290, 24, Justification::centred);
    g.setColour(Colour(0xff555f65));
    g.drawRoundedRectangle(32, 465, 557, 145, 4, 1);
    g.drawRoundedRectangle(606, 465, 350, 145, 4, 1);
    g.setColour(Colour(0xffbcc2c1));
    g.setFont(FontOptions(13.f, Font::bold));
    g.drawText("T O N E", 49, 447, 100, 25, Justification::centredLeft);
    g.drawText("L I N K", 622, 447, 100, 25, Justification::centredLeft);
    g.setFont(10.f);
    g.setColour(Colour(0xff7d898f));
    g.drawText("8 SAMPLE LATENCY   |   2x ANALOG ENGINE   |   DOUBLE PRECISION", 40, 619, 916, 22,
               Justification::centred);
}
void IronEditor::showMenu()
{
    PopupMenu m;
    m.addItem(1, "Save user preset...");
    m.addItem(2, "Load user preset...");
    m.addSeparator();
    m.addItem(3, "Undo", processor.undo.canUndo());
    m.addItem(4, "Redo", processor.undo.canRedo());
    m.addSeparator();
    for (int i = 0; i < 4; ++i)
        m.addItem(10 + i, String(std::array<int, 4>{100, 125, 150, 200}[(size_t)i]) + " %");
    m.showMenuAsync(
        PopupMenu::Options().withTargetComponent(&menu),
        [safe = Component::SafePointer<IronEditor>(this)](int result)
        {
            if (!safe)
                return;
            auto &e = *safe.getComponent();
            if (result == 3)
                e.processor.undo.undo();
            if (result == 4)
                e.processor.undo.redo();
            if (result >= 10)
            {
                auto scale = std::array<float, 4>{1, 1.25f, 1.5f, 2}[(size_t)(result - 10)];
                e.setSize((int)(1200 * scale), (int)(660 * scale));
            }
            if (result == 1 || result == 2)
            {
                bool save = result == 1;
                e.chooser = std::make_unique<FileChooser>(save ? "Save IRON preset" : "Load IRON preset",
                                                          File{}, "*.ironpreset");
                e.chooser->launchAsync(
                    save ? (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles |
                            FileBrowserComponent::warnAboutOverwriting)
                         : (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles),
                    [safe, save](const FileChooser &c)
                    {
                        if (!safe || c.getResult() == File{})
                            return;
                        auto f = c.getResult();
                        bool ok = save ? safe->processor.savePreset(f.withFileExtension("ironpreset"))
                                       : safe->processor.loadPreset(f);
                        if (!ok)
                            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "IRON 2500",
                                                             "Preset could not be saved or loaded.");
                    });
            }
        });
}
