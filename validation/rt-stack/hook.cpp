// Compiles the real GI observer/readiness functions with only platform mocks.
#include "../../src/rr_rt_stack.h"
#include "../../src/rr_frame_coordinator.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <stdexcept>
using DWORD=unsigned long;
static DWORD error=0;
static DWORD GetLastError(){return error;}
static void SetLastError(DWORD e){error=e;}
template<class... Args> static void Log(const char*,Args...){error=999;}
static unsigned char renderer[0x914700]{};
static void* verifiedRenderer=renderer;
static bool rrNativeFrameEnabled=true,rrNativeLightingComplete=true;
static control_rr::RRFrameCoordinator rrNativeFrame;
static control_rr::RRRTFrame rrNativeRTFrame;
static unsigned long long frame=42;
static unsigned effects=107,calls=0;
static bool unreadable=false,throwNative=false,keepHistory=false;
static bool ReadEngineFrameSafe(unsigned long long* f,DWORD* fault){*f=frame;*fault=0;return true;}
static bool RRNativeReadRTSettings(unsigned* e){*e=effects;return !unreadable;}
namespace control_rr_reflection {struct WindowsAccess {
 static bool Read(void*,std::uintptr_t p,void* out,std::size_t n){if(unreadable)return false;std::memcpy(out,reinterpret_cast<void*>(p),n);return true;}
};}
static void Release(void* history){
 ++calls;error=18;if(throwNative)throw std::runtime_error("native release");
 if(!keepHistory)*static_cast<std::uintptr_t*>(history)=0;
}
static auto rrNativeGIReleaseOriginal=&Release;
static bool RRNativePartialActive(){return false;}
#include "gi-observer.inc"
int main(){
 auto* history=reinterpret_cast<std::uintptr_t*>(renderer+0x90eac8);
 for(unsigned fault=0;fault<7;++fault){
  frame=42;effects=107;unreadable=throwNative=keepHistory=false;
  rrNativeFrame={};assert(rrNativeFrame.Begin({42,3840,2160,2560,1440,107},true,true));
  assert(rrNativeFrame.FeatureResult(true,true));
  if(fault!=4)assert(rrNativeFrame.BeginLighting(42));
  rrNativeRTFrame.Begin(42,107,true);*history=1234;
  if(fault==1)frame=43;
  if(fault==2)unreadable=true;
  if(fault==3)keepHistory=true;
  if(fault==5)throwNative=true;
  const auto before=calls;
  bool threw=false;
  try {RRNativeHookGIRelease(fault==6?history+1:history);}catch(const std::runtime_error&){threw=true;}
  assert(calls==before+1&&threw==(fault==5));
  assert(GetLastError()==18);
  if(fault==0){
   assert(RRNativeLightingReady(42));effects=65;assert(!RRNativeLightingReady(42));effects=107;
   RRNativeHookGIRelease(history);assert(rrNativeFrame.Stopped());
  }else {assert(!rrNativeRTFrame.giObserved);if(fault!=5)assert(rrNativeFrame.Stopped());}
 }
 puts("PASS actual GI observer: native called once, exact history/frame/stage, duplicate/read failure, exception propagation, LastError and evaluation readiness");
}
