#pragma once
// Control FG P3-A: native-preserving PT reflection baseline observer.
// This header is included in probe.cpp AFTER the existing verified Steam/Epic/
// GOG module identities and AFTER its native-resource read helpers.
//
// SECURITY / RENDERING INVARIANTS:
// - No D3D12 commands recorded, no resource mutation, no native-raygen swap.
// - No descriptor heap/root signature/PSO hooks or replacements.
// - P3 tracing and compositing are NOT implemented in this checkpoint.
// - Only GetDesc/getNativeTexture/CPU reads on already-bound resources.
// - Never retain borrowed NativeTexture or ID3D12Resource pointers past
//   the current function call.
// - Sampling is throttled (first 8 and every 240 AA calls); no hot-path work
//   on other calls.

static std::atomic<unsigned long long> ptP3AObserverSamples{0};
static std::atomic<unsigned long long> ptP3AObserverValid{0};
static std::atomic<unsigned long long> ptP3AObserverInvalid{0};
static bool ptP3AObserverEnabled=true; // This test DLL only: observation ON.

struct P3ANativeTextureView {
    D3D12_RESOURCE_DESC desc{};
    unsigned int trackedState=0u;
    bool resourceKnown=false;
    bool stateKnown=false;
    DWORD fault=0u;
};

static bool P3ATestEnvEnabled(const wchar_t* name,bool defaultValue) noexcept {
    wchar_t text[8]{};
    const DWORD length=GetEnvironmentVariableW(name,text,_countof(text));
    if(length==0u||length>=_countof(text))return defaultValue;
    return text[0]==L'1';
}

static void P3AObserverConfigure() noexcept {
    ptP3AObserverEnabled=P3ATestEnvEnabled(L"CONTROLFG_P3_OBSERVE",true);
    const bool requestedTrace=P3ATestEnvEnabled(L"CONTROLFG_P3_TRACE",false);
    const bool requestedComposite=P3ATestEnvEnabled(L"CONTROLFG_P3_COMPOSITE",false);
    Log("P3_A_CONFIG observer=%u trace_requested=%u trace_enabled=0 composite_requested=%u composite_enabled=0 native_raygen_override=0 native_deferred_composite_override=0 gpu_dispatches=0 gpu_writes=0 private_gpu_resources=0 mode=read_only_native_preserving baseline=v2.1.2",
        unsigned(ptP3AObserverEnabled),unsigned(requestedTrace),unsigned(requestedComposite));
    if(requestedTrace||requestedComposite){
        Log("P3_A_UNIMPLEMENTED_FEATURE_REQUEST_BLOCKED trace_requested=%u composite_requested=%u fail_closed=1 reason=p3a_observation_only",
            unsigned(requestedTrace),unsigned(requestedComposite));
    }
}

static bool P3AReadNativeTexture(
    std::uintptr_t shaderRva,P3ANativeTextureView* view) noexcept {
    if(!view)return false;
    *view={};
    if(!verifiedRenderer||!verifiedD3d)return false;
    using NativeTextureGetter=void* (*)(void*);
    const auto get=reinterpret_cast<NativeTextureGetter>(GetProcAddress(verifiedD3d,
        "?getNativeTexture@ShaderTexture@d3d@@QEAAPEAVNativeTexture@2@XZ"));
    if(!get)return false;
    void* native=nullptr;
    DWORD getFault=0u;
    __try {
        native=get(reinterpret_cast<unsigned char*>(verifiedRenderer)+shaderRva);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        getFault=GetExceptionCode();
    }
    if(!native){
        view->fault=getFault;
        return false;
    }

    // ReadNativeTextureState reads the native state-tracker and the D3D12
    // resource's descriptor only; it does not transition any resource.
    NativeTextureStateSnapshot snapshot{};
    if(!ReadNativeTextureState(native,&snapshot)||!snapshot.resource){
        view->fault=snapshot.fault;
        return false;
    }
    view->desc=snapshot.desc;
    view->trackedState=snapshot.trackedState;
    view->stateKnown=snapshot.stateKnown!=0u;
    view->resourceKnown=true;
    return true;
}

static void P3AObserveAfterAA(
    unsigned long long aa,unsigned long long present,
    unsigned long long engineFrame) noexcept {
    if(!ptP3AObserverEnabled)return;
    // Hard cap diagnostic rate even with FG/DLSS-RR changing frame cadence.
    if(aa>8ull&&(aa%240ull)!=0ull)return;
    const auto sample=ptP3AObserverSamples.fetch_add(1ull,std::memory_order_relaxed)+1ull;

    // These are the hash-locked renderer ShaderTexture members recovered
    // from the native deferred-reflection bind:
    // material at +0x1291888, normal at +0x12918B0, position +0x12918D8.
    constexpr std::uintptr_t kMaterial=0x1291888u;
    constexpr std::uintptr_t kNormal=0x12918B0u;
    constexpr std::uintptr_t kPosition=0x12918D8u;
    P3ANativeTextureView m{},n{},p{};
    const bool material=P3AReadNativeTexture(kMaterial,&m);
    const bool normal=P3AReadNativeTexture(kNormal,&n);
    const bool position=P3AReadNativeTexture(kPosition,&p);
    const bool sameDimensions=material&&normal&&position&&
        m.desc.Width==n.desc.Width&&n.desc.Width==p.desc.Width&&
        m.desc.Height==n.desc.Height&&n.desc.Height==p.desc.Height;
    const bool nativeShape=sameDimensions&&
        m.desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&
        n.desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&
        p.desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&
        m.desc.DepthOrArraySize==2u&&
        n.desc.DepthOrArraySize==2u&&
        p.desc.DepthOrArraySize==2u&&
        m.desc.Format==DXGI_FORMAT_R16_UINT&&
        n.desc.Format==DXGI_FORMAT_R16G16B16A16_FLOAT&&
        p.desc.Format==DXGI_FORMAT_R16G16B16A16_FLOAT;
    if(nativeShape)ptP3AObserverValid.fetch_add(1ull,std::memory_order_relaxed);
    else ptP3AObserverInvalid.fetch_add(1ull,std::memory_order_relaxed);

    EngineCommandContextSnapshot command{};
    const bool commandValid=ReadEngineCommandContext(nullptr,&command);
    // This optional renderer-local context is not necessarily available at
    // the AA boundary. A missing queue is telemetry only, not a failure.
    RRGuideInputSnapshot rendererContext{};
    const char* reason="unavailable";
    const bool queueValid=RRGuideReadRendererContext(&rendererContext,&reason);

    Log("P3_A_NATIVE_SNAPSHOT sample=%llu aa=%llu present=%llu engine_frame=%llu phase=after_native_aa material_found=%u normal_found=%u position_found=%u native_two_layer_shape=%u same_dimensions=%u material=%llux%u_layers%u_mips%u_fmt%u_flags0x%X normal=%llux%u_layers%u_mips%u_fmt%u_flags0x%X position=%llux%u_layers%u_mips%u_fmt%u_flags0x%X states=0x%X_0x%X_0x%X state_known=%u_%u_%u native_shader_override=0 gpu_writes=0",
        sample,aa,present,engineFrame,
        unsigned(material),unsigned(normal),unsigned(position),
        unsigned(nativeShape),unsigned(sameDimensions),
        static_cast<unsigned long long>(m.desc.Width),m.desc.Height,
        unsigned(m.desc.DepthOrArraySize),unsigned(m.desc.MipLevels),
        unsigned(m.desc.Format),unsigned(m.desc.Flags),
        static_cast<unsigned long long>(n.desc.Width),n.desc.Height,
        unsigned(n.desc.DepthOrArraySize),unsigned(n.desc.MipLevels),
        unsigned(n.desc.Format),unsigned(n.desc.Flags),
        static_cast<unsigned long long>(p.desc.Width),p.desc.Height,
        unsigned(p.desc.DepthOrArraySize),unsigned(p.desc.MipLevels),
        unsigned(p.desc.Format),unsigned(p.desc.Flags),
        m.trackedState,n.trackedState,p.trackedState,
        unsigned(m.stateKnown),unsigned(n.stateKnown),unsigned(p.stateKnown));
    Log("P3_A_COMMAND_CONTEXT sample=%llu aa=%llu present=%llu command_list_known=%u command_list=%p command_context=%p command_type=%u source_tls=%u fault=0x%08lX renderer_queue_known=%u queue=%p renderer_context=%p renderer_frame=%llu queue_reason=%s trace_enabled=0 composite_enabled=0",
        sample,aa,present,unsigned(commandValid),command.commandList,
        command.context,command.commandType,command.sourceTls,command.fault,
        unsigned(queueValid),rendererContext.queue,rendererContext.commandContext,
        rendererContext.engineFrame,reason?reason:"unknown");
}
