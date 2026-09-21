#include "transition.h"
#include <cstdio>
#include <initializer_list>
#include <cstdlib>
using namespace hdrguard;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(1);}}while(0)
Frame initial(bool hdr=false) {Frame f{};f.now=1000;f.present=100;f.displayValid=f.foreground=f.presentOK=f.allInputs=f.coreResetIdle=true;
 f.hdr=f.bridgeHdr=hdr;f.generation=f.settled=1;f.selection=2;f.colorWidth=3840;f.colorHeight=2160;f.colorFormat=f.hudlessFormat=24;return f;}
void advance(Frame& f){f.now+=17;++f.present;}
void stopping(Transition& t,Frame& f){CHECK(t.start(f));f.coreOff=true;advance(f);t.observe(f);CHECK(t.phase==Phase::Stopping);
 t.stopCommitted=true;advance(f);t.observe(f);CHECK(t.phase==Phase::Stopping);++f.frees;advance(f);t.observe(f);CHECK(t.phase==Phase::Drain);
 for(int i=0;i<3;++i){advance(f);t.observe(f);}CHECK(t.phase==Phase::Drain);f.stopFenceDone=true;advance(f);t.observe(f);CHECK(t.phase==Phase::ReplayReady);}
int main(){
 for(bool hdr:{false,true}){auto f=initial(hdr);Transition t;stopping(t,f);t.replayResult(true,f.now);CHECK(t.hold());
  for(int i=0;i<80;++i){advance(f);t.observe(f);}CHECK(t.phase==Phase::AwaitDisplay);f.hdr=f.bridgeHdr=!hdr;f.generation=f.settled=2;
  for(int i=0;i<8;++i){advance(f);t.observe(f);}CHECK(t.phase==Phase::Warming);CHECK(!t.releaseForCurrentFrame(f));
  for(int i=0;i<60;++i){advance(f);t.observe(f);}CHECK(t.phase==Phase::WarmFence);CHECK(t.hold());
  f.warmFenceDone=true;advance(f);t.observe(f);CHECK(t.phase==Phase::ResumeReady);CHECK(t.releaseForCurrentFrame(f));CHECK(!t.hold());
  advance(f);t.observe(f);CHECK(t.phase==Phase::Confirming);f.coreOff=false;++f.generated;advance(f);t.observe(f);CHECK(t.phase==Phase::Idle);
 }
 {auto f=initial();Transition t;CHECK(t.start(f));CHECK(!t.start(f));f.hdr=true;advance(f);t.observe(f);CHECK(t.phase==Phase::Failed);CHECK(t.hold());}
 {auto f=initial();Transition t;stopping(t,f);f.foreground=false;t.observe(f);CHECK(t.phase==Phase::Failed);}
 {auto f=initial();Transition t;stopping(t,f);t.replayResult(false,f.now);CHECK(t.phase==Phase::Failed);t.manualOffRecovery(2);CHECK(t.hold());t.manualOffRecovery(0);CHECK(!t.active());}
 for(unsigned bad=0;bad<10;++bad){auto f=initial();Transition t;stopping(t,f);t.replayResult(true,f.now);f.hdr=f.bridgeHdr=true;f.generation=f.settled=2;
  switch(bad){case 0:f.allInputs=false;break;case 1:f.colorWidth=0;break;case 2:f.colorHeight=0;break;case 3:f.colorFormat=0;break;case 4:f.hudlessFormat=10;break;
  case 5:f.coreResetIdle=false;break;case 6:f.settled=1;break;case 7:f.foreground=false;break;case 8:f.presentOK=false;break;case 9:f.bridgeHdr=false;break;}
  for(int i=0;i<100;++i){advance(f);t.observe(f);}CHECK(t.phase==Phase::AwaitDisplay);CHECK(t.hold());CHECK(!t.releaseForCurrentFrame(f));
 }
 {auto f=initial();Transition t;stopping(t,f);t.replayResult(true,f.now);f.hdr=f.bridgeHdr=true;f.generation=f.settled=2;
  for(int i=0;i<60;++i){advance(f);t.observe(f);}CHECK(t.phase==Phase::WarmFence);f.allInputs=false;advance(f);t.observe(f);CHECK(t.phase==Phase::AwaitDisplay);
  f.allInputs=true;f.warmFenceDone=false;for(int i=0;i<50;++i){advance(f);t.observe(f);}CHECK(t.phase==Phase::WarmFence);
  f.warmFenceDone=true;advance(f);t.observe(f);f.allInputs=false;CHECK(!t.releaseForCurrentFrame(f));f.allInputs=true;f.selection=0;CHECK(t.releaseForCurrentFrame(f));advance(f);t.observe(f);CHECK(t.phase==Phase::Idle);
 }
 {auto f=initial();Transition t;CHECK(t.start(f));f.now+=26000;t.observe(f);CHECK(t.phase==Phase::Failed);CHECK(t.hold());}
 {auto f=initial();Transition t;CHECK(t.start(f));t.stopCommitted=true;f.coreOff=true;++f.frees;advance(f);t.observe(f);CHECK(t.phase==Phase::Drain);
  f.stopFenceDone=true;advance(f);t.observe(f);CHECK(t.offPresents==2);for(int i=0;i<100;++i)t.observe(f);CHECK(t.offPresents==2);CHECK(t.phase==Phase::Drain);
  f.coreOff=false;advance(f);t.observe(f);CHECK(t.phase==Phase::Failed);}
 {auto f=initial();Transition t;stopping(t,f);t.replayResult(true,f.now);f.hdr=f.bridgeHdr=true;f.generation=f.settled=2;
  for(int i=0;i<60;++i){advance(f);t.observe(f);}f.warmFenceDone=true;advance(f);t.observe(f);CHECK(t.phase==Phase::ResumeReady);
  ++f.generation;++f.settled;CHECK(!t.releaseForCurrentFrame(f));}
 // Repeated HDR on/off with unique generations: no retained UI-owner allocation is used by this policy.
 for(unsigned cycle=0;cycle<1000;++cycle){auto f=initial(cycle%2);Transition t;stopping(t,f);t.replayResult(true,f.now);f.hdr=f.bridgeHdr=!f.hdr;f.generation=f.settled=cycle+2;
  for(int i=0;i<60;++i){advance(f);t.observe(f);}f.warmFenceDone=true;advance(f);t.observe(f);CHECK(t.releaseForCurrentFrame(f));f.coreOff=false;++f.generated;advance(f);t.observe(f);CHECK(!t.active());}
 std::printf("PASS %u checks; HDR on/off, off commit, GPU fences, replay failure, focus loss, all UI inputs, timeout, manual Off, 1000 cycles\n",checks);
}
