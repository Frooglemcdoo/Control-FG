#pragma once
// Experimental PT reflection P1 discovery scaffold.
//
// Scope is deliberately limited to the two build-locked direct calls in the
// native RT-reflection pass:
//   renderer+0x12953C -> renderer+0x1D9960 ("rt_reflection.rfx")
//   renderer+0x12954B -> renderer+0x1D2CE0 ("rt_reflectionDeferredShading")
//
// Both hooks call the native helper exactly once and return its exact result.
// No renderer object, shader bytecode, state object, descriptor, or GPU command
// is modified in P1-discovery mode. The captured pointers establish the exact
// shader object that a later P1 replacement must substitute fail-closed.

using PTReflectionRfxLookupFn = void* (*)(void*, const char*);
using PTReflectionTechniqueLookupFn = void* (*)(void*, const char*);

static PTReflectionRfxLookupFn ptReflectionOriginalRfxLookup=nullptr;
static PTReflectionTechniqueLookupFn ptReflectionOriginalTechniqueLookup=nullptr;
static RRAlbedoCallPatch ptReflectionLookupPatches[2]{};
static thread_local bool ptReflectionLookupScope=false;
static thread_local void* ptReflectionLastRfxObject=nullptr;
static std::atomic<unsigned long long> ptReflectionRfxLookups{0};
static std::atomic<unsigned long long> ptReflectionTechniqueLookups{0};

static void* PTReflectionHookRfxLookup(void* manager,const char* name) {
    char safeName[128]{};
    RRSafeCopyCString(name,safeName,sizeof(safeName));
    const bool target=strcmp(safeName,"rt_reflection.rfx")==0;
    void* result=ptReflectionOriginalRfxLookup(manager,name);
    if(target){
        ptReflectionLookupScope=true;
        ptReflectionLastRfxObject=result;
        const auto call=++ptReflectionRfxLookups;
        if(call<=16 || (call%240)==0)
            Log("PT_REFLECTION_P1_RFX_LOOKUP call=%llu manager=%p name=%s result=%p mode=observe_only replacement=disabled native_result_preserved=1",
                call,manager,safeName,result);
    } else {
        ptReflectionLookupScope=false;
        ptReflectionLastRfxObject=nullptr;
    }
    return result;
}

static void* PTReflectionHookTechniqueLookup(void* rfx,const char* technique) {
    char safeTechnique[128]{};
    RRSafeCopyCString(technique,safeTechnique,sizeof(safeTechnique));
    const bool target=ptReflectionLookupScope &&
        rfx==ptReflectionLastRfxObject &&
        strcmp(safeTechnique,"rt_reflectionDeferredShading")==0;
    void* result=ptReflectionOriginalTechniqueLookup(rfx,technique);
    if(target){
        const auto call=++ptReflectionTechniqueLookups;
        Log("PT_REFLECTION_P1_TECHNIQUE call=%llu rfx=%p technique=%s shader_object=%p mode=observe_only replacement=disabled native_result_preserved=1",
            call,rfx,safeTechnique,result);
    }
    ptReflectionLookupScope=false;
    ptReflectionLastRfxObject=nullptr;
    return result;
}

static bool PTReflectionInstallShaderLookupHooks(HMODULE renderer) noexcept {
    if(!renderer || renderer!=verifiedRenderer)return false;
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    ptReflectionOriginalRfxLookup=reinterpret_cast<PTReflectionRfxLookupFn>(base+0x1D9960);
    ptReflectionOriginalTechniqueLookup=reinterpret_cast<PTReflectionTechniqueLookupFn>(base+0x1D2CE0);

    if(!RRAlbedoPrepareCallPatch(base+0x12953C,
        reinterpret_cast<void*>(ptReflectionOriginalRfxLookup),
        reinterpret_cast<void*>(&PTReflectionHookRfxLookup),
        &ptReflectionLookupPatches[0]))return false;
    if(!RRAlbedoPrepareCallPatch(base+0x12954B,
        reinterpret_cast<void*>(ptReflectionOriginalTechniqueLookup),
        reinterpret_cast<void*>(&PTReflectionHookTechniqueLookup),
        &ptReflectionLookupPatches[1]))return false;

    unsigned applied=0;
    for(;applied<2;++applied){
        if(!RRAlbedoExchangeCall(&ptReflectionLookupPatches[applied],true))break;
        if(!ptReflectionLookupPatches[applied].writeHealthy){++applied;break;}
    }
    if(applied!=2){
        while(applied){
            --applied;
            if(!RRAlbedoExchangeCall(&ptReflectionLookupPatches[applied],false))
                Log("PT_REFLECTION_P1_HOOK_ROLLBACK_FAILED index=%u",applied);
        }
        return false;
    }
    Log("PT_REFLECTION_P1_LOOKUP_HOOKS ready=1 rfx_callsite=0x12953C technique_callsite=0x12954B rfx_helper=0x1D9960 technique_helper=0x1D2CE0 replacement=disabled gpu_work=unchanged");
    return true;
}
