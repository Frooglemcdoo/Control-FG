#include "../../src/rr_rt_stack.h"
#include "../../src/rr_frame_coordinator.h"
#include <cassert>
#include <cstdio>
using namespace control_rr;
int main(){
 for(unsigned effects=0;effects<256;++effects){
  assert(RRSupportsRTSettings(effects)==((effects&1)!=0&&effects<128));
  if(!RRSupportsRTSettings(effects))continue;
  RRRTFrame f;f.Begin(8,effects,true);
  assert(!f.Complete(9,effects,true)&&!f.Complete(8,effects^32,true)&&!f.Complete(8,effects,false));
  assert(f.Complete(8,effects,true)==!(effects&RTDiffuseGI));
  if(effects&RTDiffuseGI){
   assert(!f.ObserveGI(7,true,true,true)&&!f.ObserveGI(8,false,true,true));
   assert(!f.ObserveGI(8,true,false,true)&&!f.ObserveGI(8,true,true,false));
   assert(!f.giObserved&&f.ObserveGI(8,true,true,true));
   assert(!f.ObserveGI(8,true,true,true)&&f.Complete(8,effects,true));
  }else assert(!f.ObserveGI(8,true,true,true));
  f.Begin(9,effects,true);assert(!f.giObserved);
  f.Begin(9,effects,false);assert(!f.ObserveGI(9,true,true,true)&&!f.Complete(9,effects,true));
 }
 RRFrameCoordinator c;
 for(unsigned frame=1;frame<2001;++frame){
  const unsigned effects=(frame%4<2)?107:65;
  assert(c.Begin({frame,3840,2160,2560,1440,effects},true,true));
  assert(c.FeatureResult(true,true));assert(c.Reset()==(frame==1||frame%2==0));
  assert(c.BeginLighting(frame)&&c.BeginEvaluation(frame,true,true)&&c.EvaluationResult(true));
 }
 assert(!c.Begin({2001,3840,2160,2560,1440,107},false,true));
 assert(c.FeatureResult(true,false)&&c.NeedsHistoryReset());
 assert(c.HistoryResetResult(true)&&c.CompleteSR());
 assert(c.Begin({2002,3840,2160,2560,1440,107},true,true));
 assert(c.FeatureResult(true,true)&&c.Reset());
 puts("PASS 64 admitted RT combinations; GI/frame/settings rejection; 2000 settings changes; OFF/history-reset/ON");
}
