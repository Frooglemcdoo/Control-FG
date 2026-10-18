from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/rr_gi_history.h').read_text();hook=s[s.index('inline void GIObserveSet'):]
fixture=r'''
#include <windows.h>
#include <atomic>
#include <cassert>
struct ID3D12GraphicsCommandList {};
struct ID3D12PipelineState {};
struct GIBundle {ID3D12PipelineState* native=nullptr;ID3D12PipelineState* replacement=nullptr;unsigned kind=0;};
std::atomic<GIBundle*> giBundles[24]{};
struct GIScope {ID3D12GraphicsCommandList* list=nullptr;ID3D12PipelineState* observed=nullptr;unsigned temporal=0,spatial=0,missed=0;};
thread_local GIScope* giActive=nullptr;
ID3D12PipelineState* current=nullptr;ID3D12PipelineState* dispatched=nullptr;unsigned calls=0;bool raiseDispatch=false;
void setOriginal(ID3D12GraphicsCommandList*,ID3D12PipelineState* p){current=p;}
void dispatchOriginal(ID3D12GraphicsCommandList*,UINT x,UINT y,UINT z){assert(x==4&&y==3&&z==1);++calls;dispatched=current;if(raiseDispatch)RaiseException(0xe0123456,0,0,nullptr);}
HOOK
int main(){
 ID3D12GraphicsCommandList list,foreign;ID3D12PipelineState native,replacement,unknown;
 GIBundle b{&native,&replacement,0};giBundles[0].store(&b);GIScope scope;giActive=&scope;
 GIObserveSet(&list,&native);current=&native;
 assert(GIDispatch(&list,4,3,1)&&calls==1&&dispatched==&replacement&&current==&native&&scope.temporal==1);
 b.kind=1;assert(GIDispatch(&list,4,3,1)&&scope.spatial==1&&current==&native);
 assert(!GIDispatch(&foreign,4,3,1)&&scope.missed==1&&calls==2);
 GIObserveSet(&list,&unknown);assert(!GIDispatch(&list,4,3,1)&&scope.missed==2&&calls==2);
 GIObserveSet(&list,&native);raiseDispatch=true;bool caught=false;
 __try {GIDispatch(&list,4,3,1);}
 __except(GetExceptionCode()==0xe0123456?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught=true;}
 assert(caught&&current==&native&&calls==3);
 giActive=nullptr;assert(!GIDispatch(&list,4,3,1)&&calls==3);
}
'''.replace('HOOK',hook)
with tempfile.TemporaryDirectory() as td:
 d=Path(td);src=d/'dispatch.cpp';src.write_text(fixture);exe=d/'dispatch.exe'
 subprocess.run(['cl','/nologo','/std:c++17','/EHsc','/W4','/WX',str(src),'/Fe'+str(exe),'/Fo'+str(d/'dispatch.obj')],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS production GI dispatch: matching PSO only, foreign list/unknown/off pass-through, exact PSO restoration including exception')
