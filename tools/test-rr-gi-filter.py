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
namespace control_rr_native_clamp {struct Lease {unsigned original=0;};}
struct Api {
 bool Arm(control_rr_native_clamp::Lease& l){l.original=provider;if(!armOkay)return false;provider=1;SetLastError(900);return true;}
 bool Restore(control_rr_native_clamp::Lease& l){++restores;provider=l.original;SetLastError(901);return restoreOkay;}
} rrNativeClampApi;
void native(void* c,void* h,unsigned p){++calls;seen=provider;seenColor=c;seenHistory=h;seenPasses=p;
 SetLastError(321);if(raiseNative)RaiseException(0xe0123456,0,0,nullptr);}
auto rrNativeDiffuseFilterOriginal=&native;
template<class... T> void Log(const char*,T...){SetLastError(902);}
HOOK
void reset(){rrNativeFrame={};rrNativeRTFrame.Begin(240,3,true);rrNativeFrameEnabled=true;
 partial=false;armOkay=restoreOkay=true;raiseNative=false;provider=0;seen=99;calls=restores=0;SetLastError(123);}
int main(){
 void* c=reinterpret_cast<void*>(0x12340000);void* h=reinterpret_cast<void*>(0x1090eac8);
 for(unsigned original=0;original<2;++original){reset();provider=original;
  RRNativeHookGIFilter(c,h,6);assert(calls==1&&seen==1&&provider==original&&restores==1);
  assert(seenColor==c&&seenHistory==h&&seenPasses==6&&GetLastError()==321&&!rrNativeFrame.failed);
 }
 for(int mode=0;mode<4;++mode){reset();
  if(mode==0)rrNativeFrameEnabled=false;if(mode==1)rrNativeFrame.selected=false;
  if(mode==2)rrNativeFrame.stopped=true;if(mode==3)partial=true;
  RRNativeHookGIFilter(c,h,4);assert(calls==1&&seen==0&&restores==0&&!rrNativeFrame.failed);
 }
 for(int mode=0;mode<3;++mode){reset();void* arg=h;
  if(mode==0)armOkay=false;if(mode==1)arg=nullptr;if(mode==2)rrNativeRTFrame.effects=1;
  RRNativeHookGIFilter(c,arg,4);assert(calls==1&&seen==0&&restores==0&&rrNativeFrame.failed);
 }
 reset();restoreOkay=false;RRNativeHookGIFilter(c,h,6);assert(rrNativeFrame.failed&&restores==1);
 reset();raiseNative=true;bool caught=false;
 __try {RRNativeHookGIFilter(c,h,6);}
 __except(GetExceptionCode()==0xe0123456?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught=true;}
 assert(caught&&calls==1&&seen==1&&provider==0&&restores==1);
}
'''.replace('HOOK',hook)
with tempfile.TemporaryDirectory() as td:
 d=Path(td);src=d/'filter.cpp';src.write_text(fixture);exe=d/'filter.exe'
 subprocess.run(['cl','/nologo','/std:c++17','/EHsc','/W4','/WX','/I'+str(root/'src'),str(src),'/Fe'+str(exe),'/Fo'+str(d/'filter.obj')],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS production GI filter: scoped flag, exact arguments, OFF/PARTIAL, rejection, restore failure, native SEH unwind')
