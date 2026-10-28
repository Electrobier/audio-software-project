#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace psy {
constexpr double pi=3.14159265358979323846;
inline double hz(double note) {return 440.0*std::exp2((note-69.0)/12.0);}
struct Params {
 double tune=0,fine=0,cutoff=400,resonance=.5,env=.55,decay=.16,accent=.6,slide=.07,drive=1.8,volume=.55;
 int wave=0,oversampling=2;
};
struct Step {int note=36; bool active=true,accent=false,slide=false; double gate=.65;};
struct Pattern {std::array<Step,64> steps{}; int length=16,transpose=0; double division=.25,swing=0;};
struct Preset {std::string name; Params params; Pattern pattern;};
inline std::array<Preset,32> factories() {
 std::array<Preset,32> a;
 constexpr int intervals[4][8]={{0,0,7,0,12,3,0,10},{0,0,0,7,0,0,12,0},{0,1,0,7,-12,0,6,3},{0,13,7,1,10,19,3,-5}};
 const char* names[32]={"Ion Storm","Phosphor","Solar Coil","Acid Bloom","Fractal Bite","Neon Serpent","Proton Rush","Liquid Fire","Orbit Runner","Pulse Garden","Elastic Roots","Helix Motion","Silver Stream","Photon Train","Gravity Roll","Sunrise Grid","Abyss Signal","Obsidian","Night Circuit","Sub Ritual","Shadow Root","Black Spiral","Deep Canopy","Lunar Engine","Bent Geometry","Broken Prism","Quantum Steps","Odd Creature","Voltage Maze","Mutant Orbit","Chromatic Rain","Event Horizon"};
 for(int n=0;n<32;++n){auto& p=a[n];int c=n/8,v=n%8;p.name=names[n];
 p.params.cutoff=(c==2?80:160)+v*65+c*37;p.params.resonance=(c==1?.18:.53)+v*.035;
 p.params.env=(c==1?.25:.5)+v*.045;p.params.decay=(c==1?.065:.12)+v*.022;
 p.params.accent=.35+v*.075;p.params.slide=.035+v*.016;p.params.drive=1.2+c*.7+v*.21;p.params.wave=(v+c)%2;p.params.volume=.5;
 p.pattern.swing=(c==3?.035*v:0);p.pattern.length=c==3?12+v%5:16;
 for(int i=0;i<64;++i){auto& s=p.pattern.steps[i];s.note=(c==2?28:36)+intervals[c][(i+v)%8]+(v%3==2&&i%7==0?12:0);
 s.active=c==1?i%4!=0:((i*3+v)%11!=4);s.accent=(i+v)%(c==1?4:3)==0;s.slide=c!=1&&(i+2*v)%5==3;s.gate=c==1?.48:.58+((i+v)%3)*.12;}
 }return a;
}
// PolyBLEP removes the discontinuity at each saw/square edge. Continuous phase.
inline double blep(double t,double dt){if(t<dt){t/=dt;return 2*t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+2*t+1;}return 0;}
struct Oscillator {double phase=0; double tick(double frequency,double sr,double wave){double dt=std::clamp(frequency/sr,1e-8,.45);double saw=2*phase-1-blep(phase,dt);double square=(phase<.5?1.0:-1.0)+blep(phase,dt)-blep(std::fmod(phase+.5,1.0),dt);double x=saw+(square-saw)*std::clamp(wave,0.0,1.0);phase+=dt;phase-=std::floor(phase);return x;}};
// Three trapezoidal one-poles with an implicit nonlinear feedback solve.
// Each stage: y=G*u+s; state'=2*y-state. A bounded Newton solve handles
// tanh(input-k*y3), avoiding a delayed high-Q feedback loop. Nominal 18 dB/oct.
struct Filter {
 std::array<double,3> z{};
 double tick(double input,double cutoff,double resonance,double sr,double drive){
 double g=std::tan(pi*std::clamp(cutoff,20.0,sr*.20)/sr),G=g/(1+g),a=1-G;
 double b=G*G*a*z[0]+G*a*z[1]+a*z[2],h=G*G*G;
 double k=std::clamp(resonance,0.0,1.0)*7.3, u=std::tanh(input*drive-k*b);
 for(int i=0;i<5;++i){double t=std::tanh(input*drive-k*(h*u+b));double d=1+k*h*(1-t*t);u-=std::clamp((u-t)/d,-1.0,1.0);}
 double y=u;for(auto& s:z){double v=G*y+a*s;s=2*v-s;if(std::abs(s)<1e-24)s=0;y=v;}
 return y*(1+resonance*.6/(1+cutoff/1800));
 }
};
// 33-tap Blackman-windowed half-band decimator. Coefficients are computed
// during prepare, never in the callback. Cascading gives 4x -> 2x -> 1x.
struct Decimator {
 std::array<double,33> c{},z{};int head=0;
 void prepare(){double sum=0;for(int i=0;i<33;++i){int n=i-16;double sinc=n==0?.5:std::sin(pi*.5*n)/(pi*n);double window=.42-.5*std::cos(2*pi*i/32)+.08*std::cos(4*pi*i/32);c[size_t(i)]=sinc*window;sum+=c[size_t(i)];}for(auto&v:c)v/=sum;z.fill(0);head=0;}
 void push(double x){z[size_t(head)]=x;head=(head+1)%33;}
 double output()const{double y=0;int j=(head+32)%33;for(int i=0;i<33;++i){y+=c[size_t(i)]*z[size_t(j)];j=(j+32)%33;}return y;}
};
class Voice {
 double sr=48000, pitch=36,target=36, amp=0, env=0,acc=0,velocity=1,dcx=0,dcy=0;
 bool gate=false;Oscillator osc;Filter filter;
 double waveSmooth=0,bendSmooth=0,modSmooth=0;Params smooth{};Decimator dec1,dec2;std::array<double,13> delay{};int delayHead=0,previousOs=2;double lastOutput=0,transition=0;
public:
 Params params; double bend=0,mod=0;
 void prepare(double rate){sr=rate;dec1.prepare();dec2.prepare();reset();smooth=params;waveSmooth=params.wave;bendSmooth=bend;modSmooth=mod;previousOs=params.oversampling;}
 void reset(){osc={};filter={};amp=env=acc=dcx=dcy=0;gate=false;pitch=target=36;dec1.z.fill(0);dec2.z.fill(0);delay.fill(0);delayHead=0;lastOutput=transition=0;}
 void on(int note,double vel,bool legato=false,bool accent=false){target=std::clamp(note,0,127);velocity=std::clamp(vel,0.0,1.0);if(!legato||!gate){pitch=target;env=1;acc=accent?1:0;}else if(accent) acc=1;gate=true;}
 void off(){gate=false;}
 double currentPitch()const{return pitch;}
 double amplitude()const{return amp;}
 float tick(){
 double sm=1-std::exp(-1/(sr*.008));
#define SM(x) smooth.x+=(params.x-smooth.x)*sm
 SM(tune);SM(fine);SM(cutoff);SM(resonance);SM(env);SM(decay);SM(accent);SM(slide);SM(drive);SM(volume);
#undef SM
 pitch+=(target-pitch)*(1-std::exp(-1/(sr*std::max(.001,smooth.slide))));
 amp+=((gate?velocity:0)-amp)*(1-std::exp(-1/(sr*(gate?.002:.009))));
 env*=std::exp(-1/(sr*std::max(.015,smooth.decay)));acc*=std::exp(-1/(sr*.16));
 int os=params.oversampling==4?4:(params.oversampling==2?2:1);double y=0;
 waveSmooth+=(params.wave-waveSmooth)*sm;bendSmooth+=(bend-bendSmooth)*sm;modSmooth+=(mod-modSmooth)*sm;
 double f=hz(pitch+smooth.tune+smooth.fine/100+bendSmooth),fc=smooth.cutoff*std::exp2(6*smooth.env*smooth.env*env+1.6*acc*smooth.accent+2*modSmooth);
 for(int i=0;i<os;++i){double x=osc.tick(f,sr*os,waveSmooth);double high=filter.tick(x,fc,smooth.resonance,sr*os,smooth.drive);if(os==1)y=high;else if(os==2){dec2.push(high);if(i==0)y=dec2.output();}else{dec1.push(high);if(i%2==0){dec2.push(dec1.output());if(i==0)y=dec2.output();}}}
 // Constant 12-sample latency across all modes: FIR group delays are 8 / 12.
 delay[size_t(delayHead)]=y;int pad=os==1?12:os==2?4:0;y=delay[size_t((delayHead+13-pad)%13)];delayHead=(delayHead+1)%13;
 if(os!=previousOs){transition=1;previousOs=os;}if(transition>0){double mix=1-transition;y=lastOutput*(1-mix)+y*mix;transition=std::max(0.0,transition-1/(sr*.003));}lastOutput=y;
 y=y*amp*(1+acc*smooth.accent*.65)*smooth.volume*.6;
 double dc=y-dcx+std::exp(-2*pi*12/sr)*dcy;dcx=y;dcy=dc;
 if(!std::isfinite(dc)){reset();return 0;}return float(std::tanh(dc));
 }
};
// Absolute musical position determines steps: no accumulated clock drift.
// Swing lengthens even steps and shortens odd steps within a fixed pair.
struct Position {int step=0;int64_t cycle=0;double phase=0;};
inline Position position(double ppq,const Pattern& p){double d=std::max(.015625,p.division),v=std::max(0.0,ppq)/d;auto pair=int64_t(std::floor(v/2));double rem=v-pair*2;double split=1+std::clamp(p.swing,0.0,.49);int64_t absolute=pair*2+(rem>=split?1:0);double phase=rem<split?rem/split:(rem-split)/(2-split);int len=std::clamp(p.length,1,64);return {int(absolute%len),absolute/len,phase};}
class Sequencer {
 int last=-1;int64_t cycle=-1;bool sounding=false,previousSlide=false;
public:
 int visible=-1;
 void reset(Voice& v){last=-1;cycle=-1;sounding=false;previousSlide=false;visible=-1;v.off();}
 void tick(Voice& v,const Pattern& p,double ppq,bool discontinuity=false){auto pos=position(ppq,p);if(discontinuity)reset(v);
 if(pos.step!=last||pos.cycle!=cycle){const auto& s=p.steps[pos.step];bool legato=sounding&&previousSlide&&s.active;
 if(!legato)v.off();if(s.active)v.on(std::clamp(s.note+p.transpose,0,127),s.accent?1:.78,legato,s.accent);
 sounding=s.active;previousSlide=s.slide;last=visible=pos.step;cycle=pos.cycle;}
 auto& s=p.steps[pos.step];if(sounding&&!s.slide&&pos.phase>=s.gate){v.off();sounding=false;}
 }
};
struct KickParams {double tune=48,start=180,pitchDecay=.025,decay=.23,click=.25,drive=1.5,level=.7;};
inline KickParams kickPreset(int n){return {38.0+n*2.5,130.0+n*17,.012+n*.003,.13+n*.025,.08+n*.06,1+n*.23,.7};}
struct Kick {KickParams p;double phase=0,t=100;uint32_t rng=1;void trigger(){t=0;phase=0;rng=1;}float tick(double sr){double f=p.tune+(p.start-p.tune)*std::exp(-t/p.pitchDecay);phase+=2*pi*f/sr;rng=1664525*rng+1013904223;double noise=double(rng)/4294967296.0*2-1;double x=std::sin(phase)*std::exp(-t/p.decay)*std::min(1.0,t/.0007)+noise*p.click*std::exp(-t/.0018);t+=1/sr;return float(std::tanh(x*p.drive)*p.level);}};

inline void savePreset(const std::filesystem::path& file,const Preset& p){std::ofstream o(file);if(!o)throw std::runtime_error("Cannot write preset");o<<std::setprecision(17)<<"PSY303 1\n"<<std::quoted(p.name)<<'\n';auto& v=p.params;
 o<<v.tune<<' '<<v.fine<<' '<<v.cutoff<<' '<<v.resonance<<' '<<v.env<<' '<<v.decay<<' '<<v.accent<<' '<<v.slide<<' '<<v.drive<<' '<<v.volume<<' '<<v.wave<<' '<<v.oversampling<<'\n';
 o<<p.pattern.length<<' '<<p.pattern.transpose<<' '<<p.pattern.division<<' '<<p.pattern.swing<<'\n';for(auto&s:p.pattern.steps)o<<s.note<<' '<<s.active<<' '<<s.accent<<' '<<s.slide<<' '<<s.gate<<'\n';if(!o)throw std::runtime_error("Preset write failed");}
inline Preset loadPreset(const std::filesystem::path& f){std::ifstream in(f);std::string magic;int version;Preset p;in>>magic>>version;if(magic!="PSY303"||version!=1)throw std::runtime_error("Invalid preset version");in>>std::quoted(p.name);auto&v=p.params;in>>v.tune>>v.fine>>v.cutoff>>v.resonance>>v.env>>v.decay>>v.accent>>v.slide>>v.drive>>v.volume>>v.wave>>v.oversampling;
 in>>p.pattern.length>>p.pattern.transpose>>p.pattern.division>>p.pattern.swing;for(auto&s:p.pattern.steps)in>>s.note>>s.active>>s.accent>>s.slide>>s.gate;
 if(!in||p.pattern.length<1||p.pattern.length>64||!std::isfinite(v.cutoff)||v.cutoff<20||v.cutoff>20000||!std::isfinite(p.pattern.division)||p.pattern.division<=0)throw std::runtime_error("Invalid preset data");
 for(auto&s:p.pattern.steps)if(s.note<0||s.note>127||!std::isfinite(s.gate)||s.gate<.05||s.gate>1)throw std::runtime_error("Invalid step");return p;}
inline void put16(std::ostream&o,uint16_t v){o.put(char(v));o.put(char(v>>8));}
inline void put32(std::ostream&o,uint32_t v){put16(o,uint16_t(v));put16(o,uint16_t(v>>16));}
inline void wav(const std::filesystem::path& f,const std::vector<float>& samples,int rate,int bits=24,int channels=1){if((bits!=24&&bits!=32)||(channels!=1&&channels!=2)||samples.size()>500000000)throw std::runtime_error("WAV settings");std::ofstream o(f,std::ios::binary);if(!o)throw std::runtime_error("Cannot write WAV");uint32_t bytes=uint32_t(samples.size()*channels*(bits/8));o.write("RIFF",4);put32(o,36+bytes);o.write("WAVEfmt ",8);put32(o,16);put16(o,bits==32?3:1);put16(o,uint16_t(channels));put32(o,uint32_t(rate));put32(o,uint32_t(rate*channels*bits/8));put16(o,uint16_t(channels*bits/8));put16(o,uint16_t(bits));o.write("data",4);put32(o,bytes);for(float x:samples)for(int c=0;c<channels;++c){x=std::clamp(x,-.99999f,.99999f);if(bits==32){uint32_t b;static_assert(sizeof(x)==sizeof(b));std::memcpy(&b,&x,4);put32(o,b);}else{int32_t v=int32_t(std::lround(x*8388607));o.put(char(v));o.put(char(v>>8));o.put(char(v>>16));}}if(!o)throw std::runtime_error("WAV write failed");}
struct MidiEvent{int tick;std::array<uint8_t,3> data;};
inline void midi(const std::filesystem::path& f,const Pattern& p,double bpm=145){std::vector<MidiEvent> e;auto add=[&](int t,int a,int b,int c){e.push_back({t,{uint8_t(a),uint8_t(b),uint8_t(c)}});};
 for(int i=0;i<p.length;++i){auto&s=p.steps[i];if(!s.active)continue;double start=(i/2*2+(i%2?1+p.swing:0))*p.division;double len=p.division*(i%2?1-p.swing:1+p.swing);int t=int(std::lround(start*960));int end=int(std::lround((start+len*(s.slide?1.01:s.gate))*960));int note=std::clamp(s.note+p.transpose,0,127);add(t,0xb0,65,s.slide?127:0);add(t,0xb0,80,s.accent?127:0);add(t,0x90,note,s.accent?127:99);add(end,0x80,note,0);}
 std::stable_sort(e.begin(),e.end(),[](auto&a,auto&b){return a.tick<b.tick;});std::ostringstream track(std::ios::binary);auto vlq=[&](int v){uint8_t b[5];int n=0;b[n++]=uint8_t(v&127);while((v>>=7)>0)b[n++]=uint8_t((v&127)|128);while(n)track.put(char(b[--n]));};int us=int(60000000/bpm);track.write("\0\xff\x51\x03",4);track.put(char(us>>16));track.put(char(us>>8));track.put(char(us));int last=0;for(auto&a:e){vlq(a.tick-last);last=a.tick;for(auto b:a.data)track.put(char(b));}vlq(std::max(0,int(std::lround(p.length*p.division*960))-last));track.write("\xff\x2f\0",3);auto data=track.str();std::ofstream o(f,std::ios::binary);o.write("MThd\0\0\0\6\0\0\0\1\3\xc0MTrk",18);uint32_t n=uint32_t(data.size());for(int i=3;i>=0;--i)o.put(char(n>>(8*i)));o.write(data.data(),std::streamsize(data.size()));if(!o)throw std::runtime_error("MIDI write failed");}
inline std::vector<float> render(const Preset& p,int rate=48000,int repeats=2,double bpm=145,bool tail=true){Voice v;v.params=p.params;v.prepare(rate);Sequencer s;double beats=p.pattern.length*p.pattern.division*std::clamp(repeats,1,64);int frames=int(std::ceil(beats*60/bpm*rate));std::vector<float> out(size_t(frames+(tail?rate:0)));for(size_t i=0;i<out.size();++i){if(int(i)<frames)s.tick(v,p.pattern,double(i)*bpm/(60*rate));else v.off();out[i]=v.tick();}return out;}
}
