#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Core.h"
namespace psy {
inline const std::array<const char*,20> ids={"tune","cutoff","resonance","env","decay","accent","volume","slide","drive","fine","wave","oversampling","mode","run","bpm","length","transpose","division","swing","rhythm"};
inline juce::String sid(int s,const char* field){return "s"+juce::String(s)+"_"+field;}
class Processor final: public juce::AudioProcessor {
public:
 Processor();
 juce::UndoManager undo;
 juce::AudioProcessorValueTreeState state;
 juce::MidiKeyboardState keyboard;
 std::atomic<int> currentStep{-1},currentNote{-1},learn{-1};
 std::array<std::atomic<int>,10> mappings;
 std::array<std::atomic<float>,10> ccPending;
 std::atomic<bool> panic{false};
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 void set(const juce::String&,float);
 void key(int note,float velocity,bool on){int w=queueWrite.load(std::memory_order_relaxed),next=(w+1)%256;if(next==queueRead.load(std::memory_order_acquire)){panic=true;return;}queue[size_t(w)]={note,velocity,on};queueWrite.store(next,std::memory_order_release);}
 float get(const juce::String&)const;
 Preset snapshot()const;
 void apply(const Preset&);
 void factory(int);
 void saveUser(const juce::File&);void loadUser(const juce::File&);
 void prepareToPlay(double,int)override;
 void releaseResources()override{}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
 bool isBusesLayoutSupported(const BusesLayout&)const override;
 juce::AudioProcessorEditor* createEditor()override;
 bool hasEditor()const override{return true;}
 const juce::String getName()const override{return "Psy303 Bassline";}
 bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}
 double getTailLengthSeconds()const override{return 1;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}
 const juce::String getProgramName(int)override{return "Current";}void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
private:
 struct Key{int note=0;float velocity=0;bool on=false;};
 std::array<Key,256> queue{};std::atomic<int> queueWrite{0},queueRead{0};
 std::array<std::atomic<float>*,20> raw{};
 std::array<juce::RangedAudioParameter*,10> learnedParams{};std::array<float,10> previousRaw{};std::array<std::atomic<float>,10> ccOverride;
 std::array<std::array<std::atomic<float>*,5>,64> steps{};
 Voice voice;Sequencer seq;double rate=48000,freePpq=0,expectedPpq=0;bool wasRunning=false;int previousMode=-1;
 std::array<int,128> held{};std::array<float,128> velocities{};int serial=0;bool ccSlide=false,ccAccent=false;
 void message(const juce::MidiMessage&);void clearNotes();
};
}
