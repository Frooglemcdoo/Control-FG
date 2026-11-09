#pragma once
#include "fg_source_policy.h"
// Track full-resource writes into registered HDR shadows, then accept them only
// once their command list is submitted to the bridge's direct queue. This is CPU
// submission evidence; GPU ordering comes from using that same queue.
static SRWLOCK fgSourceLock=SRWLOCK_INIT;
static control_fg_source::Tracker fgSourceTracker;
struct FGSourceHook {void** table=nullptr;void* original=nullptr;};
static FGSourceHook fgSourceCopyHooks[8]{},fgSourceResetHooks[8]{},fgSourceQueueHooks[8]{};
using FGSourceCopy=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12Resource*,ID3D12Resource*);
using FGSourceReset=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
using FGSourceExecute=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
static void* FGSourceOriginal(FGSourceHook* hooks,void** table) noexcept {
    void* result=nullptr;AcquireSRWLockShared(&fgSourceLock);
    for(unsigned i=0;i<8;++i)if(hooks[i].table==table)result=hooks[i].original;
    ReleaseSRWLockShared(&fgSourceLock);return result;
}
static void STDMETHODCALLTYPE FGSourceCopyHook(ID3D12GraphicsCommandList* list,ID3D12Resource* dst,ID3D12Resource* src) {
    auto fn=reinterpret_cast<FGSourceCopy>(FGSourceOriginal(fgSourceCopyHooks,*reinterpret_cast<void***>(list)));
    if(!fn)return;fn(list,dst,src);
    AcquireSRWLockExclusive(&fgSourceLock);fgSourceTracker.Record(list,dst);ReleaseSRWLockExclusive(&fgSourceLock);
}
static HRESULT STDMETHODCALLTYPE FGSourceResetHook(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso) {
    auto fn=reinterpret_cast<FGSourceReset>(FGSourceOriginal(fgSourceResetHooks,*reinterpret_cast<void***>(list)));
    if(!fn)return E_UNEXPECTED;const HRESULT hr=fn(list,allocator,pso);
    if(SUCCEEDED(hr)){AcquireSRWLockExclusive(&fgSourceLock);fgSourceTracker.Reset(list);ReleaseSRWLockExclusive(&fgSourceLock);}return hr;
}
static void STDMETHODCALLTYPE FGSourceExecuteHook(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists) {
    auto fn=reinterpret_cast<FGSourceExecute>(FGSourceOriginal(fgSourceQueueHooks,*reinterpret_cast<void***>(queue)));
    if(!fn)return;
    control_fg_source::Submission last{};
    AcquireSRWLockExclusive(&fgSourceLock);
    for(UINT i=0;i<count;++i){auto candidate=fgSourceTracker.Submitted(lists[i]);if(candidate.generation)last=candidate;}
    ReleaseSRWLockExclusive(&fgSourceLock);
    fn(queue,count,lists);
    AcquireSRWLockExclusive(&fgSourceLock);fgSourceTracker.Commit(last,queue==GetHdr10BridgeDirectQueue());ReleaseSRWLockExclusive(&fgSourceLock);
}
static bool FGSourceInstall(FGSourceHook* hooks,void** table,unsigned slot,void* replacement) noexcept {
    // Exclusive lock held. Publish original before replacement becomes callable.
    for(unsigned i=0;i<8;++i)if(hooks[i].table==table)return true;
    for(unsigned i=0;i<8;++i)if(!hooks[i].table){
        hooks[i]={table,table[slot]};
        if(FGPixelPatch(&table[slot],replacement))return true;
        hooks[i]={};return false;
    }
    return false;
}
static void FGSourceObserveList(ID3D12GraphicsCommandList* list) noexcept {
    auto queue=GetHdr10BridgeDirectQueue();if(!queue||!list)return;
    AcquireSRWLockExclusive(&fgSourceLock);
    const bool q=FGSourceInstall(fgSourceQueueHooks,*reinterpret_cast<void***>(queue),10,reinterpret_cast<void*>(&FGSourceExecuteHook));
    const bool r=FGSourceInstall(fgSourceResetHooks,*reinterpret_cast<void***>(list),10,reinterpret_cast<void*>(&FGSourceResetHook));
    const bool c=q&&r&&FGSourceInstall(fgSourceCopyHooks,*reinterpret_cast<void***>(list),17,reinterpret_cast<void*>(&FGSourceCopyHook));
    ReleaseSRWLockExclusive(&fgSourceLock);
    if(!c){static bool warned=false;if(!warned){warned=true;Log("FG_SOURCE_TRACKER hook_install_failed=1 action=dxgi_source_fallback");}}
}
static void FGSourceRegister(const void* owner,ID3D12Resource* buffer,UINT index) noexcept {
    AcquireSRWLockExclusive(&fgSourceLock);const bool ok=fgSourceTracker.Register(owner,buffer,index);ReleaseSRWLockExclusive(&fgSourceLock);
    Log("FG_SOURCE_REGISTER owner=%p buffer=%p index=%u success=%u",owner,buffer,index,unsigned(ok));
}
static void FGSourceClear(const void* owner) noexcept {
    AcquireSRWLockExclusive(&fgSourceLock);fgSourceTracker.Clear(owner);ReleaseSRWLockExclusive(&fgSourceLock);
}
static UINT FGSourceTake(const void* owner,UINT fallback,bool& observed) noexcept {
    AcquireSRWLockExclusive(&fgSourceLock);const UINT result=fgSourceTracker.Take(owner,fallback,observed);ReleaseSRWLockExclusive(&fgSourceLock);return result;
}
