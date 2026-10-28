#pragma once
#include "Plugin.h"
#include <thread>
namespace psy {
class MetalLook final:public juce::LookAndFeel_V4 {
public:void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;
};
class StepGrid final:public juce::Component {
 Processor& p;int page=0;public:int selected=0;std::function<void(int)> selection;
 explicit StepGrid(Processor&proc):p(proc){}void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&)override;void setPage(int v){page=v;selected=v*16;repaint();if(selection)selection(selected);}
};
class KeyView final:public juce::MidiKeyboardComponent {
 Processor& p;
public:explicit KeyView(Processor&v):MidiKeyboardComponent(v.keyboard,horizontalKeyboard),p(v){}
 void drawWhiteNote(int n,juce::Graphics&g,juce::Rectangle<float>r,bool down,bool over,juce::Colour line,juce::Colour text)override{MidiKeyboardComponent::drawWhiteNote(n,g,r,down||p.currentNote==n,over,line,text);}
 void drawBlackNote(int n,juce::Graphics&g,juce::Rectangle<float>r,bool down,bool over,juce::Colour fill)override{MidiKeyboardComponent::drawBlackNote(n,g,r,down||p.currentNote==n,over,fill);}
};
class Editor final:public juce::AudioProcessorEditor,private juce::Timer,private juce::MidiKeyboardStateListener {
 Processor& p;MetalLook look;StepGrid grid;
 KeyView keys;
 std::array<juce::Slider,10> knobs;std::array<juce::Label,10> labels;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderLinks;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboLinks;
 juce::ComboBox presets,wave,os,mode,division,rhythm,page,learnTarget,exportType,kickSelect;
 juce::Slider tempo,length,transpose,swing,stepNote,stepGate,repeats;
 juce::ToggleButton tail{"EXPORT TAIL"};juce::TextButton quickSave{"SAVE"};juce::File lastFile;
 juce::TextButton kickEdit{"KICK EDIT"};juce::Component::SafePointer<juce::DialogWindow> kickWindow;
 juce::TextButton play{"PLAY / STOP"},panic{"PANIC"},save{"SAVE AS"},load{"LOAD"},prev{"<"},next{">"},init{"INIT"},undo{"UNDO"},redo{"REDO"},exportButton{"EXPORT..."},learnButton{"MIDI LEARN"},octDown{"OCT -"},octUp{"OCT +"};
 juce::Label status,stepLabel;
 std::unique_ptr<juce::FileChooser> chooser;
 std::thread worker;std::atomic<bool> busy{false};std::atomic<int> result{0};
 int octave=3,program=0;bool refreshing=false;float fine=0;
 void timerCallback()override;void selectStep(int);void exportFile();void userFile(bool saving);
 void handleNoteOn(juce::MidiKeyboardState*,int,int note,float velocity)override{p.key(note,velocity,true);if(p.get("mode")>.5f)p.set(sid(grid.selected,"note"),float(note));}
 void handleNoteOff(juce::MidiKeyboardState*,int,int note,float)override{p.key(note,0,false);}
 void attach(juce::Slider&,const juce::String&);void attach(juce::ComboBox&,const juce::String&);
public:explicit Editor(Processor&);~Editor()override;void paint(juce::Graphics&)override;void resized()override;
};
}
