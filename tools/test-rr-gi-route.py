"""Execute the production GI option hook with native calls mocked."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/rr_native_frame.h').read_text()
hook=s[s.index('static bool RRNativeGIBypassOption('):s.index('static bool RRNativeLightingReady(')]
fixture=r'''
#include <cassert>
#include <cstdint>
#include "rr_rt_stack.h"
using DWORD=unsigned long;
namespace control_rr {enum class RRFrameStage {Lighting,Other};}
struct Frame {
 bool selected=true,stopped=false,reset=false,failed=false;
 control_rr::RRFrameStage stage=control_rr::RRFrameStage::Lighting;
 bool Selected(){return selected;} bool Stopped(){return stopped;}
 bool Reset(){return reset;} auto Stage(){return stage;}
 void Fail(const char*){failed=true;}
} rrNativeFrame;
control_rr::RRRTFrame rrNativeRTFrame;
bool rrNativeFrameEnabled=true,partial=false,readable=true;
unsigned bridgeCalls=0,fallbackCalls=0;DWORD error=123;
void* verifiedRenderer=reinterpret_cast<void*>(0x10000000);
bool RRNativePartialActive(){return partial;}
DWORD GetLastError(){return error;} void SetLastError(DWORD v){error=v;}
bool ReadEngineFrameSafe(unsigned long long* f,DWORD*){*f=8;error=42;return readable;}
bool RRNativeSelectedOption(void*){++fallbackCalls;return rrNativeFrameEnabled&&(rrNativeFrame.selected||partial)&&!rrNativeFrame.stopped;}
void PTReflectionBridgeDiffuseGI(){++bridgeCalls;error=99;}
template<class... T> void Log(const char*,T...){error=65;}
HOOK
void reset(){rrNativeFrame={};rrNativeFrameEnabled=true;partial=false;readable=true;
 bridgeCalls=fallbackCalls=0;error=123;rrNativeRTFrame.Begin(8,3,true);}
int main(){
 void* option=reinterpret_cast<void*>(0x10914570);
 reset();assert(!RRNativeGIBypassOption(option));
 assert(bridgeCalls==1&&fallbackCalls==0&&error==123&&!rrNativeFrame.failed);
 assert(rrNativeRTFrame.Complete(8,3,true));
 assert(!RRNativeGIBypassOption(option));assert(bridgeCalls==1&&rrNativeFrame.failed);
 for(int mode=0;mode<4;++mode){reset();
  if(mode==0)rrNativeFrameEnabled=false;
  if(mode==1)rrNativeFrame.selected=false;
  if(mode==2)rrNativeFrame.stopped=true;
  if(mode==3)partial=true;
  RRNativeGIBypassOption(option);assert(fallbackCalls==1&&bridgeCalls==0&&!rrNativeRTFrame.giObserved);
 }
 for(int mode=0;mode<5;++mode){reset();void* arg=option;
  if(mode==0)arg=nullptr;
  if(mode==1)readable=false;
  if(mode==2)rrNativeFrame.stage=control_rr::RRFrameStage::Other;
  if(mode==3)rrNativeRTFrame.Begin(9,3,true);
  if(mode==4)rrNativeRTFrame.Begin(8,1,true);
  assert(!RRNativeGIBypassOption(arg));assert(rrNativeFrame.failed&&bridgeCalls==0&&!rrNativeRTFrame.giObserved&&error==123);
 }
}
'''.replace('HOOK',hook)
with tempfile.TemporaryDirectory() as td:
 d=Path(td);src=d/'route.cpp';src.write_text(fixture)
 exe=d/('route.exe' if os.name=='nt' else 'route')
 if os.name=='nt':cmd=['cl','/nologo','/std:c++17','/EHsc','/W4','/WX','/I'+str(root/'src'),str(src),'/Fe'+str(exe),'/Fo'+str(d/'route.obj')]
 else:cmd=['g++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(root/'src'),str(src),'-o',str(exe)]
 subprocess.run(cmd,check=True);subprocess.run([str(exe)],check=True)
print('PASS production GI route: bridge exactly once, native branch, fallback, invalid frame/option/stage, last-error preservation')
