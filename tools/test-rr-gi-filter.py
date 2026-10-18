"""Run the production GI filter wrapper on Windows, including SEH unwind."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/rr_native_frame.h').read_text()
hook=s[s.index('static void RRNativeHookGIFilter('):s.index('static void RRNativeHookContactShadowFilter(')]
fixture=r'''
#include <windows.h>
#include <cassert>
#include <cstdint>
#include "rr_rt_stack.h"
struct Frame {bool selected=true,stopped=false,failed=false;
 bool Selected(){return selected;} bool Stopped(){return stopped;} bool Reset(){return true;}
 void Fail(const char*){failed=true;}} rrNativeFrame;
control_rr::RRRTFrame rrNativeRTFrame;
bool rrNativeFrameEnabled=true,partial=false,armOkay=true,restoreOkay=true,raiseNative=false;
unsigned provider=0,seen=99,calls=0,restores=0;
void* verifiedRenderer=reinterpret_cast<void*>(0x10000000);
void* seenColor=nullptr;void* seenHistory=nullptr;unsigned seenPasses=0;
bool RRNativePartialActive(){return partial;}
namespace control_rr_clamp {
struct GIScope {unsigned temporal=0,spatial=0,missed=0,passthrough=0;};
GIScope* giActive=nullptr;
bool GIReady(){return armOkay;}
}
void release(void*){++restores;SetLastError(222);}
auto rrNativeGIReleaseOriginal=&release;
void native(void* c,void* h,unsigned p){++calls;seenColor=c;seenHistory=h;seenPasses=p;
 if(control_rr_clamp::giActive){++control_rr_clamp::giActive->temporal;control_rr_clamp::giActive->spatial=p;control_rr_clamp::giActive->passthrough=4;if(!restoreOkay)control_rr_clamp::giActive->missed=1;}
 SetLastError(321);if(raiseNative)RaiseException(0xe0123456,0,0,nullptr);}
auto rrNativeDiffuseFilterOriginal=&native;
template<class... T> void Log(const char*,T...){SetLastError(902);}
HOOK
void reset(){rrNativeFrame={};rrNativeRTFrame.Begin(240,3,true);rrNativeFrameEnabled=true;
 partial=false;armOkay=restoreOkay=true;raiseNative=false;provider=0;seen=99;calls=restores=0;SetLastError(123);}
int main(){
 void* c=reinterpret_cast<void*>(0x12340000);void* h=reinterpret_cast<void*>(0x1090eac8);
 reset();RRNativeHookGIFilter(c,h,6);assert(calls==1&&seenColor==c&&seenHistory==h&&seenPasses==6&&GetLastError()==321&&!rrNativeFrame.failed&&!control_rr_clamp::giActive);
 for(int mode=0;mode<4;++mode){reset();
  if(mode==0)rrNativeFrameEnabled=false;if(mode==1)rrNativeFrame.selected=false;
  if(mode==2)rrNativeFrame.stopped=true;if(mode==3)partial=true;
  RRNativeHookGIFilter(c,h,4);assert(calls==1&&restores==0&&!rrNativeFrame.failed&&!control_rr_clamp::giActive);
 }
 reset();armOkay=false;RRNativeHookGIFilter(c,h,6);assert(calls==0&&restores==1&&!rrNativeFrame.failed&&GetLastError()==222);
 reset();RRNativeHookGIFilter(c,nullptr,6);assert(calls==1&&rrNativeFrame.failed);
 reset();restoreOkay=false;RRNativeHookGIFilter(c,h,6);assert(rrNativeFrame.failed&&!control_rr_clamp::giActive);
 reset();raiseNative=true;bool caught=false;
 __try {RRNativeHookGIFilter(c,h,6);}
 __except(GetExceptionCode()==0xe0123456?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught=true;}
 assert(caught&&calls==1&&!control_rr_clamp::giActive);

}
'''.replace('HOOK',hook)
with tempfile.TemporaryDirectory() as td:
 d=Path(td);src=d/'filter.cpp';src.write_text(fixture);exe=d/'filter.exe'
 subprocess.run(['cl','/nologo','/std:c++17','/EHsc','/W4','/WX','/I'+str(root/'src'),str(src),'/Fe'+str(exe),'/Fo'+str(d/'filter.obj')],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS production GI filter: scoped flag, exact arguments, OFF/PARTIAL, rejection, restore failure, native SEH unwind')
