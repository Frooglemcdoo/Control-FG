"""Execute the production vtable registry with two different COM implementations.
The reported regression was compute-first installation rejecting direct lists.
This tests actual registry/installer C++ with OS patching and locks stubbed only.
"""
from pathlib import Path
import subprocess, tempfile, shutil, os
root=Path(__file__).resolve().parents[1]
s=(root/'src/pt_reflection_third_runtime.h').read_text()
registry=s[s.index('struct PTThirdVtable {'):s.index('static PTThirdCreateListFn ptThirdCreateList')]
installer=s[s.index('static bool PTThirdInstallVtable('):s.index('static bool PTThirdInstallList(')]
fresh=s[s.index('static bool PTThirdConsumeFresh()'):s.index('static HRESULT STDMETHODCALLTYPE PTThirdHookCreateList')]
preamble=r'''
#include <atomic>
#include <cassert>
#include <cstdio>
using SRWLOCK=int;
#define SRWLOCK_INIT 0
void AcquireSRWLockShared(SRWLOCK*){} void ReleaseSRWLockShared(SRWLOCK*){}
void AcquireSRWLockExclusive(SRWLOCK*){} void ReleaseSRWLockExclusive(SRWLOCK*){}
struct ID3D12GraphicsCommandList { void** table; };
void PTThirdHookReset(){} void PTReflectionHookSetPipelineState(){}
void PTReflectionHookSetComputeRootSignature(){} void PTThirdHookTable(){}
void PTThirdHookCBV(){} void PTThirdHookSetState(){} void PTThirdHookDispatch(){}
void Log(const char*,...){}
using DWORD=unsigned long;
unsigned long long currentFrame=1;
bool ReadEngineFrameSafe(unsigned long long* out,DWORD*){*out=currentFrame;return true;}
std::atomic<unsigned long long> ptThirdWrittenFrame{0},ptThirdConsumed{0},ptThirdDispatches{0};
int computeCalls=0,directCalls=0;
void ComputeOriginal(){++computeCalls;} void DirectOriginal(){++directCalls;}
void** failSlot=nullptr;
bool PTAuditPatch(void** slot,void* hook,void** original){
    if(slot==failSlot)return false;
    if(*slot==hook)return *original!=nullptr;
    if(*original&&*original!=*slot)return false;
    *original=*slot;*slot=hook;return true;
}
'''
main=r'''
int main(){
    assert(!PTThirdConsumeFresh());
    ptThirdWrittenFrame=1;ptThirdDispatches=1;
    assert(PTThirdConsumeFresh());assert(!PTThirdConsumeFresh());
    currentFrame=2;assert(!PTThirdConsumeFresh());
    ptThirdDispatches=2;assert(!PTThirdConsumeFresh());
    ptThirdWrittenFrame=2;assert(PTThirdConsumeFresh());
    assert(!PTThirdConsumeFresh());
    void* compute[77];void* direct[77];
    for(int i=0;i<77;++i){compute[i]=(void*)&ComputeOriginal;direct[i]=(void*)&DirectOriginal;}
    ID3D12GraphicsCommandList c{compute},d{direct};
    // Reproduce observed order: compute succeeds, then a distinct direct vtable.
    assert(PTThirdInstallVtable(compute,true));
    assert(PTThirdInstallVtable(direct,true));
    for(unsigned slot:{10u,25u,29u,31u,37u,75u,76u}){
        assert(PTThirdOriginal(&c,slot)==(void*)&ComputeOriginal);
        assert(PTThirdOriginal(&d,slot)==(void*)&DirectOriginal);
        reinterpret_cast<void(*)()>(PTThirdOriginal(&c,slot))();
        reinterpret_cast<void(*)()>(PTThirdOriginal(&d,slot))();
    }
    assert(computeCalls==7&&directCalls==7);
    // Repeat installation must never record our hook as its own original.
    assert(PTThirdInstallVtable(direct,true));
    assert(PTThirdOriginal(&d,76)==(void*)&DirectOriginal);
    // A later wrapper keeps our hook as its next call. Never repoint our
    // original to that wrapper or replace the top of its chain.
    void* original=PTThirdOriginal(&d,25);
    direct[25]=(void*)&ComputeOriginal;
    assert(PTThirdInstallVtable(direct,true));
    assert(direct[25]==(void*)&ComputeOriginal);
    assert(PTThirdOriginal(&d,25)==original);
    // A distinct base interface only exposes base slots.
    void* base[60];for(auto& x:base)x=(void*)&DirectOriginal;
    assert(PTThirdInstallVtable(base,false));
    // Failed patching stays failed, while any published hooks retain originals.
    void* failed[77];for(auto& x:failed)x=(void*)&ComputeOriginal;
    failSlot=&failed[29];assert(!PTThirdInstallVtable(failed,true));
    assert(!PTThirdInstallVtable(failed,true));
    ID3D12GraphicsCommandList f{failed};assert(PTThirdOriginal(&f,10)==(void*)&ComputeOriginal);
    puts("PASS: compute-first/direct-second, distinct original routing, repeat install, external hook-chain preservation, stale-frame and duplicate-consumer rejection, partial failure");
}
'''
# initializer_list is required by the braced range in main.
source=preamble+'\n#include <initializer_list>\n'+registry+installer+fresh+main
with tempfile.TemporaryDirectory() as td:
    td=Path(td);cpp=td/'hooks.cpp';cpp.write_text(source)
    exe=td/('hooks.exe' if os.name=='nt' else 'hooks')
    if os.name=='nt':
        subprocess.run(['cl','/nologo','/std:c++17','/EHsc',str(cpp),'/Fe'+str(exe),'/Fo'+str(td/'hooks.obj')],check=True)
    else:
        subprocess.run([shutil.which('g++') or 'clang++','-std=c++17','-Wall','-Wextra',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
