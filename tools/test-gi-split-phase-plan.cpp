#include "../src/gi_reuse/split_phase_plan.h"
#include <stdexcept>
#include <cstdio>
static void Check(bool b){if(!b)throw std::runtime_error("split phase protocol");}
int main(){using P=gi_reuse::GISplitPhasePlan;try {
 for(unsigned size:{2u,3u,17u,1920u*1080u,2560u*1440u})for(unsigned parity:{0u,1u})for(unsigned fail:{0u,1u,2u}){
  P p;Check(!p.Prepare(9,size,parity,false));Check(p.CanRunFullNative());
  Check(p.Prepare(9,size,parity,true));Check(!p.BeginTraining(8));Check(p.BeginTraining(9));Check(!p.CanRunFullNative());
  if(fail!=1){Check(p.TrainingShaded(9));}
  Check(p.BeginCompletion(9,fail==0));
  Check(p.SelectedCompletion()==(fail?P::Completion::NativeComplement:P::Completion::GuidedComplement));
  Check(!p.BeginCompletion(9,false));Check(!p.BeginTraining(9));Check(!p.Publish(9,false));
  unsigned a=0,b=0;for(unsigned i=0;i<size;++i){Check(p.TrainingPixel(i)!=p.ComplementPixel(i));a+=p.TrainingPixel(i);b+=p.ComplementPixel(i);}
  for(unsigned i=0;i<a;++i)Check(p.TrainingPixel(p.TrainingPixelAt(i)));
  for(unsigned i=0;i<b;++i)Check(p.ComplementPixel(p.ComplementPixelAt(i)));
  Check(p.TrainingPixelAt(a)==UINT32_MAX&&p.ComplementPixelAt(b)==UINT32_MAX);
  Check(a==p.TrainingCount()&&b==p.ComplementCount()&&a+b==size);
  Check(!p.TrainingPixel(size)&&!p.ComplementPixel(size));Check(p.Publish(9,true));Check(!p.Publish(9,true));Check(!p.CanRunFullNative());
 }
 puts("PASS split phase: disjoint coverage, odd extents, native complement fallback, invocation identity, single publication");return 0;
 }catch(const std::exception& e){puts(e.what());return 1;}}
