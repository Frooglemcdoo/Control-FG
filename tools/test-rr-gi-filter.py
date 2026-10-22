"""Compile the production GI filter hook against an instrumented native call."""
from pathlib import Path
import subprocess, tempfile, shutil
root=Path(__file__).resolve().parents[1]
s=(root/'src/rr_native_frame.h').read_text()
hook=s[s.index('static void RRNativeHookGIFilter('):s.index('static bool RRNativeLightingReady(')]
preamble=r'''
#include "rr_rt_stack.h"
#include "rr_lighting_diagnostic.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
using DWORD=unsigned long;
static DWORD lastError=0;
static DWORD GetLastError(){return lastError;}
static void SetLastError(DWORD e){lastError=e;}
namespace control_rr {enum class RRFrameStage {Lighting,Ready};}
struct Frame {
 bool selected=true,stopped=false;control_rr::RRFrameStage stage=control_rr::RRFrameStage::Lighting;
 bool Selected(){return selected;} auto Stage(){return stage;} bool Reset(){return true;}
 void Fail(const char*){stopped=true;}
};
static Frame rrNativeFrame;
static control_rr::RRRTFrame rrNativeRTFrame;
static unsigned char renderer[0x90eb00]{};
static void* verifiedRenderer=renderer;
static bool rrNativeFrameEnabled=true,rrNativeGIKeepThisFrame=true;
static unsigned long long currentFrame=42;
static bool readable=true,throwNative=false,keepHistory=true;
static unsigned calls=0;static void* seenColor=nullptr;static void* seenHistory=nullptr;static unsigned seenPasses=0;
static bool ReadEngineFrameSafe(unsigned long long* f,DWORD*){*f=currentFrame;return true;}
namespace control_rr_reflection {struct WindowsAccess {
 static bool Read(void*,std::uintptr_t p,void* out,std::size_t n){if(!readable)return false;std::memcpy(out,reinterpret_cast<void*>(p),n);return true;}
};}
namespace native_lighting {template<class... T> void Trace(const char*,T...){lastError=99;}}
static void Native(void* color,void* history,unsigned passes){
 ++calls;seenColor=color;seenHistory=history;seenPasses=passes;lastError=18;
 assert(!rrNativeRTFrame.giObserved); // Readiness must not be asserted before native execution.
 if(throwNative)throw std::runtime_error("native failure");
 *static_cast<std::uintptr_t*>(history)=keepHistory?1234:0;
}
static auto rrNativeDiffuseFilterOriginal=&Native;
'''
main=r'''
int main(){
 auto* history=reinterpret_cast<std::uintptr_t*>(renderer+0x90eac8);
 for(unsigned fault=0;fault<9;++fault){
  rrNativeFrame={};rrNativeRTFrame.Begin(42,107,true,true);currentFrame=42;
  readable=keepHistory=true;throwNative=false;void* color=renderer;unsigned passes=4;
  auto* inputHistory=history;
  if(fault==1)currentFrame=43;
  if(fault==2)readable=false;
  if(fault==3)keepHistory=false;
  if(fault==4)rrNativeFrame.stage=control_rr::RRFrameStage::Ready;
  if(fault==5)throwNative=true;
  if(fault==6)inputHistory=history+1;
  if(fault==7)passes=0;
  if(fault==8)color=nullptr;
  unsigned before=calls;bool threw=false;
  try{RRNativeHookGIFilter(color,inputHistory,passes);}catch(const std::runtime_error&){threw=true;}
  assert(calls==before+1 && seenColor==color && seenHistory==inputHistory && seenPasses==passes);
  assert(threw==(fault==5) && GetLastError()==18);
  assert(rrNativeRTFrame.Complete(42,107,true)==(fault==0));
  if(fault!=0&&fault!=5)assert(rrNativeFrame.stopped);
 }
 rrNativeRTFrame.Begin(42,107,true,true);rrNativeFrame={};currentFrame=42;
 assert(!rrNativeRTFrame.ObserveGI(42,true,true,true)); // Bypass cannot satisfy filtered evidence.
 assert(rrNativeRTFrame.ObserveFilteredGI(42,true,true,true,4));
 assert(!rrNativeRTFrame.ObserveFilteredGI(42,true,true,true,4));
 rrNativeRTFrame.Begin(42,107,true);
 assert(!rrNativeRTFrame.ObserveFilteredGI(42,true,true,true,4));
 assert(rrNativeRTFrame.ObserveGI(42,true,true,true));
 rrNativeRTFrame.Begin(42,107,false);rrNativeGIKeepThisFrame=false;
 unsigned before=calls;RRNativeHookGIFilter(renderer,history,4);
 assert(calls==before+1&&!rrNativeRTFrame.giObserved&&GetLastError()==18);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);cpp=p/'test.cpp';cpp.write_text(preamble+hook+main)
 cl=shutil.which('cl');exe=p/('test.exe' if cl else 'test')
 command=([cl,'/nologo','/std:c++17','/EHsc','/W4','/I'+str(root/'src'),str(cpp),'/Fe'+str(exe),'/Fo'+str(p/'test.obj')] if cl else ['g++','-std=c++17','-I'+str(root/'src'),str(cpp),'-o',str(exe)])
 subprocess.run(command,check=True,cwd=p)
 subprocess.run([str(exe)],check=True)
print('PASS production GI filter: exact forwarding, native-first readiness, history/frame/stage/pass validation, duplicate rejection, exceptions and LastError')
