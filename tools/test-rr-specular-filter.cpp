#include "../src/rr_specular_noisy.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <iostream>
using namespace control_rr_specular;
struct Fixture {
 Context context{42,101,102,103,104};
 unsigned char source[0x100]{},target[0x100]{};
 bool readable=true,stateOkay=true,alias=false,badDevice=false;
 unsigned copies=0,calls=0;
 Address Source(){return reinterpret_cast<Address>(source);}
 Address Target(){return reinterpret_cast<Address>(target);}
 Fixture(){Address a=201,b=202;std::memcpy(source+0x88,&a,sizeof(a));std::memcpy(target+0x88,&b,sizeof(b));}
 bool StateMatches(Address s){return stateOkay&&s==123;}
 bool CopyNative(Address,Address){++copies;return true;}
 Access Callbacks(){
  Access a{};a.context=this;
  a.read=[](void* p,Address address,void* out,std::size_t size){auto& f=*static_cast<Fixture*>(p);if(!f.readable)return false;std::memcpy(out,reinterpret_cast<void*>(address),size);return true;};
  a.shaderNative=[](void* p,Address address){auto& f=*static_cast<Fixture*>(p);return address==1+TemporalSourceRva||f.alias?f.Source():f.Target();};
  a.describe=[](void* p,Address,Shape& s,Address& device){auto& f=*static_cast<Fixture*>(p);s={2560,1440,10,1,1,1,0,0,3};device=f.badDevice?999:f.context.device;return true;};
  a.currentContext=[](void* p,Context& c){c=static_cast<Fixture*>(p)->context;return true;};
  return a;
 }
 Scope NewScope(){Scope s{};s.frame=context;s.sourceNative=Source();s.rrCommitted=true;s.spatialPasses=4;s.preserveNativeFiltering=s.nativeFilterBound=true;return s;}
};
int main(){
 for(unsigned fault=0;fault<12;++fault){
  Fixture f;auto s=f.NewScope();Address site=TemporalDispatchRva;
  if(fault==1)f.context.frame++;
  if(fault==2)f.readable=false;
  if(fault==3)f.stateOkay=false;
  if(fault==4)f.alias=true;
  if(fault==5)f.badDevice=true;
  if(fault==6)s.nativeFilterBound=false;
  if(fault==7)s.rrCommitted=false;
  if(fault==8)s.attempted=true;
  if(fault==9)site++;
  if(fault==10)s.sourceNative++;
  if(fault==11)s.preserveNativeFiltering=false;
  ObserveFilteredDispatch(s,f.Callbacks(),f,1,site,123,[&]{++f.calls;assert(!s.recorded&&s.failed);});
  assert(f.calls==1&&f.copies==0);
  assert(s.nativeFilterValidated==(fault==0));
  assert(s.recorded==(fault==0)&&s.failed==(fault!=0));
 }
 Fixture f;auto s=f.NewScope();
 assert(!ValidateReferenceDispatch(s,f.Callbacks(),f,1,TemporalDispatchRva,123)); // Default clamp cannot acquire spatial scope.
 assert(!Substitute(s,f.Callbacks(),f,1,TemporalDispatchRva,123)&&f.copies==0);
 s.spatialPasses=0;assert(!Substitute(s,f.Callbacks(),f,1,TemporalDispatchRva,123));
 s=f.NewScope();bool threw=false;
 try{ObserveFilteredDispatch(s,f.Callbacks(),f,1,TemporalDispatchRva,123,[]{throw std::runtime_error("native");});}
 catch(const std::runtime_error&){threw=true;}
 assert(threw&&s.failed&&!s.recorded);
 s=f.NewScope();ObserveFilteredDispatch(s,f.Callbacks(),f,1,TemporalDispatchRva,123,[]{});
 assert(s.recorded&&!s.failed);
 ObserveFilteredDispatch(s,f.Callbacks(),f,1,TemporalDispatchRva,123,[]{});
 assert(s.failed); // Duplicate dispatch cannot turn a previous success into current readiness.
 s=f.NewScope();s.preserveNativeFiltering=false;s.spatialPasses=0;
 assert(ValidateReferenceDispatch(s,f.Callbacks(),f,1,TemporalDispatchRva,123));
 assert(Substitute(s,f.Callbacks(),f,1,TemporalDispatchRva,123)&&f.copies==1); // Baseline remains supported.
 std::cout<<"PASS native specular filter: frame/resource validation, exact forwarding, native-first readiness, failure and duplicate rejection, unchanged raw baseline\n";
}
