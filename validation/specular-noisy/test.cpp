#include "../../src/rr_specular_noisy.h"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace control_rr_specular;
struct Fake {
 std::array<unsigned char,256> source{},target{};
 Context context{5,6,7,8,9};Shape shape{2560,1440,10,1,1,1,0,4,3};
 Shape targetShape=shape;
 Address sourceResource=100,targetResource=200;int copyCalls=0,contextCalls=0,stateCalls=0,targetDescriptions=0;
 bool failRead=false,failCopy=false,changeFrame=false,changeList=false,badDevice=false,wrongState=false,changeState=false,changeTargetMips=false;
 Fake(){std::memcpy(source.data()+0x88,&sourceResource,8);std::memcpy(target.data()+0x88,&targetResource,8);}
 Address Source(){return reinterpret_cast<Address>(source.data());}Address Target(){return reinterpret_cast<Address>(target.data());}
 static bool ReadBytes(void* self,Address address,void* out,std::size_t size){auto& f=*static_cast<Fake*>(self);if(f.failRead)return false;std::memcpy(out,reinterpret_cast<void*>(address),size);return true;}
 static Address Shader(void* self,Address shader){auto& f=*static_cast<Fake*>(self);return shader==0x1000+TemporalSourceRva?f.Source():f.Target();}
 static bool Describe(void* self,Address resource,Shape& out,Address& device){auto& f=*static_cast<Fake*>(self);if(resource!=100&&resource!=200)return false;out=resource==100?f.shape:f.targetShape;if(resource==200&&++f.targetDescriptions>1&&f.changeTargetMips)++out.mips;device=f.badDevice?10:8;return true;}
 static bool Current(void* self,Context& out){auto& f=*static_cast<Fake*>(self);out=f.context;++f.contextCalls;if(f.changeFrame&&f.contextCalls>1)++out.frame;if(f.changeList&&f.contextCalls>1)++out.list;return true;}
 Access Accessors(){return {this,ReadBytes,nullptr,Shader,Describe,Current};}
 Scope Frame(){Scope s;s.frame=context;s.sourceNative=Source();s.rrCommitted=true;return s;}
 bool StateMatches(Address state){return state==300&&!wrongState&&!(++stateCalls>1&&changeState);}
 bool CopyNative(Address targetNative,Address sourceNative){assert(targetNative==Target()&&sourceNative==Source());++copyCalls;return !failCopy;}
 bool Run(Scope& s){return Substitute(s,Accessors(),*this,0x1000,TemporalDispatchRva,300);}
 bool RunRef(Scope& s){return ValidateReferenceDispatch(s,Accessors(),*this,0x1000,TemporalDispatchRva,300);}
};
int main(){
 {Fake f;auto s=f.Frame();assert(f.RunRef(s)&&!s.recorded&&!s.attempted&&!s.failed&&f.copyCalls==0&&std::strcmp(s.reason,"reference_dispatch_admitted")==0);}
 {Fake f;auto s=f.Frame();f.changeList=true;assert(!f.RunRef(s)&&!s.attempted&&!s.failed&&f.copyCalls==0&&std::strcmp(s.reason,"reference_context_changed_before_dispatch")==0);}
 {Fake f;auto s=f.Frame();f.changeTargetMips=true;assert(!f.RunRef(s)&&!s.attempted&&!s.failed&&f.copyCalls==0&&std::strcmp(s.reason,"reference_pair_changed_before_dispatch")==0);}
 {Fake f;auto s=f.Frame();assert(!ValidateReferenceDispatch(s,f.Accessors(),f,0x1000,0x16ee2,300)&&f.copyCalls==0);}
 {Fake f;auto s=f.Frame();assert(f.Run(s)&&s.recorded&&s.attempted&&!s.failed&&f.copyCalls==1);assert(!f.Run(s)&&f.copyCalls==1);}
 for(int which=0;which<13;++which){Fake f;auto s=f.Frame();switch(which){
 case 0:s.rrCommitted=false;break;case 1:s.spatialPasses=1;break;case 2:s.sourceNative=1;break;
 case 3:f.failRead=true;break;case 4:f.changeFrame=true;break;case 5:f.badDevice=true;break;
 case 6:f.targetShape.width++;break;case 7:f.targetShape.format=28;break;case 8:f.targetShape.samples=2;break;
 case 9:f.source[0x68]=1;break;case 10:f.target[0x38]=0x10;break;case 11:f.wrongState=true;break;
 case 12:std::memcpy(f.target.data()+0x88,&f.sourceResource,8);break;}
 assert(!f.Run(s)&&f.copyCalls==0&&!s.recorded&&!s.attempted);}
 {Fake f;auto s=f.Frame();f.failCopy=true;assert(!f.Run(s)&&s.attempted&&s.failed&&!s.recorded&&f.copyCalls==1);assert(!f.Run(s)&&f.copyCalls==1);}
 {Fake f;auto s=f.Frame();assert(!Substitute(s,f.Accessors(),f,0x1000,0x16ee2,300)&&f.copyCalls==0);}
 // 2560x1440 supports up to 12 mips. The mip-zero copy must work regardless
 // of whether either resource has the other levels allocated.
 for(unsigned sm=1;sm<=12;++sm)for(unsigned tm=1;tm<=12;++tm){
  Fake f;f.shape.mips=sm;f.targetShape.mips=tm;auto s=f.Frame();
  // Native setup may move to another list before the replaced dispatch.
  f.context.list=106;assert(f.Run(s)&&s.recorded&&s.dispatch.list==106&&f.copyCalls==1);
 }
 for(unsigned lane=0;lane<2;++lane)for(unsigned mips:{0u,13u,UINT32_MAX}){
  Fake f;(lane?f.targetShape:f.shape).mips=mips;auto s=f.Frame();
  assert(!f.Run(s)&&f.copyCalls==0&&std::strcmp(s.reason,"pair_mip_count")==0);
 }
 for(unsigned which=0;which<4;++which){Fake f;auto s=f.Frame();
  switch(which){case 0:++f.context.frame;break;case 1:++f.context.queue;break;case 2:++f.context.device;break;case 3:++f.context.view;break;}
  assert(!f.Run(s)&&f.copyCalls==0&&std::strcmp(s.reason,"dispatch_frame_owner")==0);
 }
 {Fake f;auto s=f.Frame();f.changeList=true;assert(!f.Run(s)&&f.copyCalls==0&&std::strcmp(s.reason,"dispatch_context_changed_before_copy")==0);}
 {Fake f;auto s=f.Frame();f.changeState=true;assert(!f.Run(s)&&f.copyCalls==0&&std::strcmp(s.reason,"dispatch_context_changed_before_copy")==0);}
 {Fake f;auto s=f.Frame();f.changeTargetMips=true;assert(!f.Run(s)&&f.copyCalls==0&&std::strcmp(s.reason,"pair_changed_before_copy")==0);}
 puts("PASS reference-dispatch admission plus 144 mip-count pairs, dispatch-list handoff, frame/view/device/queue guards, malformed mips, mid-read mutation, exact copy once and no retry after faults");
}
