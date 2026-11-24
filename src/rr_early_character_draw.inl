// G-buffer pass 2 installs the primary view, viewport and Direct-queue worker
// context before renderer+1267DD. Its caller finishes this worker immediately
// afterward. Keep the mask in its own color attachment and detach scene depth.
namespace RRCharacterEarly {
struct Stamp {
    UINT64 frame=0,view=0;
    ID3D12CommandQueue* queue=nullptr;
    ID3D12GraphicsCommandList* list=nullptr;
    void* nativeDepth=nullptr;
    ID3D12Resource* depth=nullptr;
    bool submitted=false;
};
static SRWLOCK guard=SRWLOCK_INIT;
static Stamp last{};
static thread_local UINT64 submitting=0;
static std::atomic<const char*> drawReason{"waiting_for_gbuffer"};
static void Reset(ID3D12GraphicsCommandList* list) noexcept {
    AcquireSRWLockExclusive(&guard);
    if(last.list==list && !last.submitted)last={};
    ReleaseSRWLockExclusive(&guard);
}
static void Before(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const* lists) noexcept {
    submitting=0;AcquireSRWLockExclusive(&guard);
    for(UINT i=0;last.list&&lists&&i<count;++i)if(lists[i]==last.list){
        if(!last.submitted && q==last.queue)submitting=last.frame;
        else if(!last.submitted)last={};
        break;
    }
    ReleaseSRWLockExclusive(&guard);
}
static void After() noexcept {
    if(!submitting)return;
    AcquireSRWLockExclusive(&guard);
    if(last.frame==submitting)last.submitted=true;
    ReleaseSRWLockExclusive(&guard);submitting=0;
}
static Stamp Snapshot() noexcept {
    AcquireSRWLockShared(&guard);const Stamp result=last;ReleaseSRWLockShared(&guard);return result;
}
static void Publish(const Stamp& stamp) noexcept {
    AcquireSRWLockExclusive(&guard);last=stamp;ReleaseSRWLockExclusive(&guard);
}
using DrawFn=void(*)(void*);
static DrawFn original=nullptr;
static RRAlbedoCallPatch patch{};
static const char* Draw(void* pass) noexcept {
    if(!rrCharacterBlendRequested.load()||rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Recording)return "no_prepared_frame";
    auto* c=rrDiffuse;if(!c||!c->earlyCharacterReady||c->fault.load())return "no_early_target";
    RRAlbedoNativeBindings saved{};DWORD fault=0;const char* why=nullptr;
    UINT64 frame=0;void* view=nullptr;
    if(!ReadEngineFrameSafe(&frame,&fault)||c->frame!=frame||
       !control_rr_albedo::ReadPrimaryView(reinterpret_cast<UINT64>(verifiedRenderer),&view,&fault)||view!=c->primaryView)return "gbuffer_view";
    control_rr_albedo::DrawRange range{};
    if(!control_rr_albedo::prepareDrawRange(c->characterReplay,int(c->characterReplay.sourceFirst),-1,range))return "empty_character_range";
    if(!RRAlbedoNativeReadBindings(&rrAlbedoAPI,&saved,&why,&fault))return why;
    if(!saved.workerContext||saved.targetCount!=2||saved.width!=c->width||saved.height!=c->height)return "gbuffer_bindings";
    bool changed=false,drawn=false;
    Stamp stamp{frame,c->cameraView,c->queue};
    __try {
        if(*reinterpret_cast<void**>(c->renderer+0x270)!=pass)return "gbuffer_owner";
        auto& target=c->earlyCharacterTarget;
        if(!RRAlbedoNativeTextureState(target.nativeTexture,D3D12_RESOURCE_STATE_RENDER_TARGET))return "early_mask_state";
        stamp.nativeDepth=saved.depthTarget;
        stamp.depth=*reinterpret_cast<ID3D12Resource**>(static_cast<unsigned char*>(saved.depthTarget)+0x88);
        stamp.list=*reinterpret_cast<ID3D12GraphicsCommandList**>(static_cast<unsigned char*>(saved.commandContext)+0x18);
        if(!stamp.list||!stamp.depth)return "gbuffer_list_or_depth";
        const void* color=target.nativeTexture;
        changed=true;
        rrAlbedoAPI.targets(saved.deviceState,1,&color,nullptr);
        RRAlbedoNativeRestoreViewport(&rrAlbedoAPI,&saved);
        drawn=RRAlbedoDrawSafe(range.managerView(),&fault);
    } __except(EXCEPTION_EXECUTE_HANDLER){drawn=false;}
    if(changed&&!RRAlbedoNativeRestore(&rrAlbedoAPI,&saved,&fault)){RRDiffuseStop("early_mask_restore");return "early_mask_restore";}
    if(!drawn)return "early_character_draw";
    Publish(stamp);return "mask_recorded";
}
static void Hook(void* pass){
    original(pass);
    DWORD error=GetLastError();drawReason.store(Draw(pass));SetLastError(error);
}
}
static void RRInstallEarlyCharacterDraw(unsigned char* base) noexcept {
    using namespace RRCharacterEarly;
    original=reinterpret_cast<DrawFn>(base+0x112e50);
    const bool installed=RRAlbedoPrepareCallPatch(base+0x1267dd,reinterpret_cast<void*>(original),
        reinterpret_cast<void*>(&Hook),&patch)&&RRAlbedoExchangeCall(&patch,true);
    Log("GI_CHARACTER_R507 gbuffer_hook=%u",unsigned(installed));
}
