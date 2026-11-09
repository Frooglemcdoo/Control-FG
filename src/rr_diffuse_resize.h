#pragma once
// All entry points run on the native primary recording thread. Workers only
// see the targets while stage==Recording; a native join ends that CPU use.
struct RRDiffuseExtentRequest {unsigned long long frame=0;UINT width=0,height=0;DWORD thread=0;};
static RRDiffuseExtentRequest rrDiffuseExtent{};
static void RRDiffuseRequestExtent(unsigned long long frame,UINT width,UINT height) noexcept {
    rrDiffuseExtent={frame,width,height,GetCurrentThreadId()};
}
static bool RRDiffuseDestroyTarget(RRAlbedoNativeTarget* target,DWORD* fault) noexcept {
    if(!target->nativeTexture)return true;
    if(target->creationThread!=GetCurrentThreadId()||!rrAlbedoAPI.destroy||!rrAlbedoAPI.deleteMemory)return false;
    __try {
        // Exact native destruction pair, audited at d3d+0x29895/0x2989E.
        // The caller has already joined CPU workers and retired GPU commands.
        rrAlbedoAPI.destroy(target->nativeTexture);
        rrAlbedoAPI.deleteMemory(target->nativeTexture);
        *target={};return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){*fault=GetExceptionCode();return false;}
}
static bool RRDiffuseResize(RRDiffuseReplay* c,UINT width,UINT height,DWORD* fault) noexcept {
    if(c->width==width&&c->height==height&&!c->resizePending)return true;
    if(c->recordingThread!=GetCurrentThreadId()||rrDiffuseStage.load()==RRDiffuseStage::Recording)return false;
    if(!c->resizePending){
        c->resizePending=true;rrDiffuseStage.store(RRDiffuseStage::Idle,std::memory_order_release);
        Log("RR_DIFFUSE_RESIZE_PENDING old=%ux%u requested=%ux%u last_present=%llu",c->width,c->height,width,height,c->lastUsePresent);
    }
    if(!c->retireFence)return false;
    const auto completed=c->fence->GetCompletedValue();
    if(completed==UINT64_MAX){RRDiffuseStop("resize_device_removed");return false;}
    if(completed<c->retireFence)return false;
    for(auto& target:c->targets)if(!RRDiffuseDestroyTarget(&target,fault)){
        RRDiffuseStop("resize_native_destroy_failed");return false;
    }
    if(!RRDiffuseDestroyTarget(&c->characterTarget,fault)){
        RRDiffuseStop("resize_character_target_destroy_failed");return false;
    }
    c->width=width;c->height=height;c->targetCount=0;c->retireFence=0;c->resizePending=false;
    Log("RR_DIFFUSE_RESIZED width=%u height=%u completed_fence=%llu",width,height,static_cast<unsigned long long>(completed));
    return true;
}
static void RRDiffuseAfterPresent(unsigned long long present) noexcept {
    const DWORD saved=GetLastError();auto* c=rrDiffuse;
    if(!c||c->recordingThread!=GetCurrentThreadId()||!c->resizePending||c->retireFence||
       present<=c->lastUsePresent||rrDiffuseStage.load()!=RRDiffuseStage::Idle)return;
    __try {
        // Native Present has returned after the old primary/worker lists and
        // their later live-guide copies were submitted to this Direct queue.
        const auto value=++c->signal;
        if(!value||FAILED(c->queue->Signal(c->fence,value))){RRDiffuseStop("resize_fence_signal_failed");SetLastError(saved);return;}
        c->retireFence=value;
    } __except(EXCEPTION_EXECUTE_HANDLER){RRDiffuseStop("resize_fence_exception");}
    SetLastError(saved);
}
