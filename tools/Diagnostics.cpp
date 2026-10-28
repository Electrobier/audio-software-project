#include "Core.h"
#include <iostream>
int main(int argc,char**argv){using namespace psy;std::filesystem::path out=argc>1?argv[1]:"diagnostics";std::filesystem::create_directories(out);for(int wave:{0,1})for(int method:{0,1}){Oscillator osc;double phase=0;std::vector<float>a(48000);for(auto&x:a){if(method)x=float(osc.tick(5000,48000,wave)*.5);else{x=float((wave?(phase<.5?1:-1):phase*2-1)*.5);phase+=5000./48000;phase-=std::floor(phase);}}wav(out/((wave?std::string("square_"):std::string("saw_"))+(method?"blep":"naive")+".wav"),a,48000,32);}
 for(int os:{1,2,4}){Voice v;v.params.cutoff=14000;v.params.resonance=.8;v.params.drive=8;v.params.env=0;v.params.volume=.5;v.params.tune=12*std::log2(5000/hz(107));v.params.oversampling=os;v.prepare(48000);v.on(107,1);for(int i=0;i<48000;++i)v.tick();std::vector<float>a(48000);for(auto&x:a)x=v.tick();wav(out/("filter_"+std::to_string(os)+"x.wav"),a,48000,32);}
 std::cout<<"DSP diagnostics rendered\n";}
