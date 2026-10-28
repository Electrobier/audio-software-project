#include "Plugin.h"
#include <iostream>
#include <cstdlib>
#include <atomic>
std::atomic<bool> track{false};std::atomic<int> allocations{0};
void* operator new(std::size_t n){if(track)++allocations;if(void*p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
void verify(bool v,const char*s){if(!v){std::cerr<<"FAIL "<<s<<'\n';std::exit(1);}}
struct Host:juce::AudioPlayHead{double q=0,bpm=145;bool play=true;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setPpqPosition(q);p.setBpm(bpm);p.setIsPlaying(play);return p;}};
int main(int argc,char**argv){juce::ScopedJuceInitialiser_GUI init;
 psy::Processor p;p.set("mode",0);p.set("run",0);p.prepareToPlay(48000,1024);juce::AudioBuffer<float>b(2,1024);juce::MidiBuffer midi;midi.ensureSize(16384);midi.addEvent(juce::MidiMessage::noteOn(1,36,.8f),77);p.processBlock(b,midi);for(int i=0;i<77;++i)verify(b.getSample(0,i)==0,"sample accurate MIDI onset");verify(b.getMagnitude(77,947)>.001,"MIDI generates audio");
 midi.addEvent(juce::MidiMessage::noteOn(1,48,.9f),0);p.processBlock(b,midi);verify(p.currentNote==48,"last note priority");midi.addEvent(juce::MidiMessage::noteOff(1,48),0);p.processBlock(b,midi);verify(p.currentNote==36,"held note return");midi.addEvent(juce::MidiMessage::allNotesOff(1),0);p.processBlock(b,midi);verify(p.currentNote==-1,"MIDI panic");
 p.learn=1;midi.addEvent(juce::MidiMessage::controllerEvent(1,74,100),34);p.processBlock(b,midi);verify(p.mappings[1]==74,"MIDI learn without editor");juce::MemoryBlock state;p.getStateInformation(state);psy::Processor restored;restored.setStateInformation(state.getData(),int(state.getSize()));verify(restored.mappings[1]==74&&restored.get("cutoff")>1000,"learned CC state and value restore");
 p.factory(25);p.set("length",37);p.set(psy::sid(31,"note"),72);p.getStateInformation(state);restored.setStateInformation(state.getData(),int(state.getSize()));verify(restored.get("length")==37&&restored.get(psy::sid(31,"note"))==72,"APVTS complete step state");
 Host host;p.setPlayHead(&host);p.set("mode",1);p.set("length",16);p.set("division",2);for(int block:{32,64,128,256,512,1024}){p.prepareToPlay(48000,block);b.setSize(2,block);host.q=0;p.processBlock(b,midi);verify(p.currentStep==0,"host starts at step 1");host.q=2;p.processBlock(b,midi);verify(p.currentStep==8,"host seek to step 9");host.q=0;p.processBlock(b,midi);verify(p.currentStep==0,"host loop reset");}
 p.setPlayHead(nullptr);p.set("mode",0);p.set("run",0);p.prepareToPlay(48000,1024);b.setSize(2,1024);for(int i=0;i<20;++i)p.processBlock(b,midi);midi.addEvent(juce::MidiMessage::noteOn(1,40,.9f),0);allocations=0;track=true;for(int i=0;i<100;++i)p.processBlock(b,midi);track=false;verify(allocations==0,"audio callback operator new allocations");
 if(argc>1){std::unique_ptr<juce::AudioProcessorEditor>e(p.createEditor());e->setVisible(true);for(double scale:{1.,1.25,1.5,2.}){e->setSize(int(1120*scale),int(790*scale));auto image=e->createComponentSnapshot(e->getLocalBounds());juce::File file(juce::String(argv[1])+"/interface-"+juce::String(int(scale*100))+".png");auto stream=file.createOutputStream();verify(stream!=nullptr,"screenshot file");stream->setPosition(0);stream->truncate();juce::PNGImageFormat png;verify(png.writeImageToStream(image,*stream),"GUI snapshot");}}
 std::cout<<"PASS plugin: sample accurate MIDI, note priority, all notes off, MIDI learn with closed editor, APVTS restore, host seek/loop across six buffers, zero operator-new allocations in 100 callbacks"<<std::endl;
}
