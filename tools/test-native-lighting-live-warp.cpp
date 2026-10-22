#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cwchar>
#include <vector>
#include <stdexcept>
#include <string>
#include <utility>
#include <iostream>
static std::atomic<unsigned long long> presentCount{0};
static void Log(const char*, ...) {}
static bool PTReflectionSha256(const unsigned char*, size_t, char*) { return false; }
// Renderer and sidecar operations are unavailable in the standalone WARP test.
static HMODULE verifiedRenderer = nullptr;
struct RRAlbedoCallPatch {};
static bool RRAlbedoPrepareCallPatch(void*, void*, void*, RRAlbedoCallPatch*) { return false; }
static bool RRAlbedoExchangeCall(RRAlbedoCallPatch*, bool) { return false; }
static bool PTReflectionLoadSidecar(const wchar_t*, const char*, std::vector<unsigned char>&, char*, bool&) { return false; }
#define CONTROL_LIGHTING_NO_OVERLAY
#include "../src/native_lighting_live_runtime.h"
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr) { if(FAILED(hr)) throw std::runtime_error("D3D failure " + std::to_string(static_cast<unsigned long>(hr))); }
static native_lighting::spot_resolution::Extent seenExtent{};
static unsigned seenDynamic=0, seenId=0;
static float seenDistance=0;
alignas(void*) static unsigned char fakePool[0x80]{}, fakeRecord[0x48]{};
static void FakeResize(void* pool, const native_lighting::spot_resolution::Extent* size) {
    if(pool!=&fakePool) throw std::runtime_error("resize owner changed");
    seenExtent=*size;
}
static void* FakeRequest(void* pool,int w,int h,unsigned dynamic,float distance,unsigned id) {
    if(pool!=&fakePool) throw std::runtime_error("request owner changed");
    seenExtent={w,h};seenDynamic=dynamic;seenDistance=distance;seenId=id;return &fakeRecord;
}
static void TestAllocatorForwarding() {
    using namespace native_lighting;
    namespace sr=spot_resolution_runtime;
    static unsigned char fakeModule[0x1400000]{};
    sr::rendererBase=fakeModule;sr::originalResize=&FakeResize;sr::originalRequest=&FakeRequest;
    spot_resolution::Extent native{16384,2048};
    sr::ResizeAt(&fakePool,&native,fakeModule+0x1376c8);
    if(seenExtent.height!=8192 || native.height!=2048) throw std::runtime_error("atlas expansion failed");
    SelectMode(Mode::SpotResolution2x);
    auto result=sr::RequestAt(&fakePool,1024,1024,1,12.5f,77,fakeModule+0x18acba);
    if(result!=&fakeRecord || seenExtent.width!=2048 || seenExtent.height!=2048 ||
       seenDynamic!=1 || seenDistance!=12.5f || seenId!=77 || sr::Observed(selection.Read()))
        throw std::runtime_error("native request ABI forwarding failed");
    void* recordPointer=&fakeRecord;
    void** records=&recordPointer;
    std::memcpy(fakePool+0x18,&records,sizeof(records));
    int count=1, width=16384, height=8192, tile=2048;
    std::memcpy(fakePool+0x5c,&count,4);
    std::memcpy(fakePool+0x64,&width,4);std::memcpy(fakePool+0x60,&height,4);
    std::memcpy(fakeRecord+8,&recordPointer,sizeof(recordPointer));
    std::memcpy(fakeRecord+0x3c,&tile,4);std::memcpy(fakeRecord+0x40,&tile,4);
    spot_shadow_audit::Packed(&fakePool);
    if(!sr::Observed(selection.Read())) throw std::runtime_error("final packed size not confirmed");
    SelectMode(Mode::SpotResolution2x);
    sr::RequestAt(&fakePool,1024,1024,1,12.5f,77,fakeModule+0x18acba);
    tile=1024;std::memcpy(fakeRecord+0x3c,&tile,4);std::memcpy(fakeRecord+0x40,&tile,4);
    spot_shadow_audit::Packed(&fakePool);
    if(sr::Observed(selection.Read())) throw std::runtime_error("downsized tile incorrectly confirmed");
    SelectMode(Mode::Native);
    if(sr::Observed(selection.Read())) throw std::runtime_error("stale request observation");
    sr::RequestAt(&fakePool,1024,1024,1,12.5f,77,fakeModule+0x18acba);
    if(seenExtent.width!=1024) throw std::runtime_error("native restoration failed");
    SelectMode(Mode::SpotResolution2x);
    sr::RequestAt(&fakePool,1024,1024,0,12.5f,77,fakeModule+0x18e4db);
    if(seenExtent.width!=1024) throw std::runtime_error("sun request modified");
    sr::ResizeAt(&fakePool,&native,fakeModule+1);
    if(seenExtent.height!=2048) throw std::runtime_error("unrelated resize modified");
    sr::originalVsmRequest=&FakeRequest;
    result=sr::VsmRequestHook(&fakePool,512,256,0,4.25f,88);
    if(result!=&fakeRecord || seenExtent.width!=512 || seenExtent.height!=256 || seenDynamic!=0 || seenDistance!=4.25f || seenId!=88)
        throw std::runtime_error("VSM observer changed native arguments");
    sr::originalVsmRequest=nullptr;
    sr::originalResize=nullptr;sr::originalRequest=nullptr;sr::rendererBase=nullptr;
    sr::expandedPool=nullptr;SelectMode(Mode::Native);
}
static int cullingArguments[3]{};
static unsigned cullingCalls = 0;
static float cullingExpected = 0;
static float cullingExpansion = 60, cullingWithoutReflections = 1, cullingFar = 150;
static void FakeSceneCollection(void* scene) {
    if (scene != &cullingArguments[0] || cullingExpansion != cullingExpected)
        throw std::runtime_error("Scene culling arguments or expansion changed");
    ++cullingCalls;
}
static void FakeObjectCollection(void* objects, void* view, void* output) {
    if (objects != &cullingArguments[0] || view != &cullingArguments[1] ||
        output != &cullingArguments[2] || cullingWithoutReflections != cullingExpected)
        throw std::runtime_error("Object culling arguments or expansion changed");
    ++cullingCalls;
}
static void TestSpotCulling() {
    using namespace native_lighting;
    namespace sc = spot_culling_runtime;
    sc::slots[0] = {&cullingExpansion};
    sc::slots[1] = {&cullingWithoutReflections};
    sc::farDistance = &cullingFar;
    sc::originalScene = &FakeSceneCollection;
    sc::originalObjects = &FakeObjectCollection;
    sc::ready.store(true);
    readyMask.fetch_or(kRtSpotReadyBit);
    SelectMode(Mode::RtSpot);
    cullingExpected=60;
    sc::SceneHook(&cullingArguments[0]);
    cullingExpected=1;
    sc::ObjectsHook(&cullingArguments[0],&cullingArguments[1],&cullingArguments[2]);
    if (cullingCalls!=2 || cullingFar!=150) throw std::runtime_error("Culling forwarding/far plane changed");
    // No render-thread wait if another collection is updating the two options.
    AcquireSRWLockExclusive(&sc::updateLock);
    sc::Update();
    ReleaseSRWLockExclusive(&sc::updateLock);
    SelectMode(Mode::RtSpotNativeCull);
    sc::Update();
    if(cullingExpansion!=60 || cullingWithoutReflections!=1)
        throw std::runtime_error("Native culling settings not restored");
    SelectMode(Mode::RtSpot);
    sc::Update();
    cullingExpansion=180; // Later engine/debug changes must survive native restore.
    SelectMode(Mode::Native);
    sc::Update();
    if(cullingExpansion!=180 || cullingWithoutReflections!=1)
        throw std::runtime_error("External culling setting was overwritten");
    sc::ready.store(false); sc::farDistance=nullptr;
    sc::slots[0]={}; sc::slots[1]={};
    sc::originalScene=nullptr; sc::originalObjects=nullptr;
    readyMask.fetch_and(~kRtSpotReadyBit);
    std::cout<<"PASS native caster coverage: arguments, unchanged expansion, external values, no lock wait\n";
}
static bool passForwarded = false;
static int passArguments[4]{};
static void FakeSpotPass(void* a, void* b, void* c, void* d) {
    using namespace native_lighting;
    auto* scope = rt_spot_runtime::active;
    if (!scope || !scope->requested || a != &passArguments[0] || b != &passArguments[1] ||
        c != &passArguments[2] || d != &passArguments[3]) throw std::runtime_error("RT pass arguments or selection changed");
    SelectMode(Mode::Native); // UI changes cannot split the in-progress pair.
    if (!scope->requested) throw std::runtime_error("RT selection was not held for the pass");
    if (rt_spot_runtime::NeedsNativeShadowFiltering()) throw std::runtime_error("Requested but untraced pass changed filtering");
    scope->raySeen = true;
    if (rt_spot_runtime::NeedsNativeShadowFiltering()) throw std::runtime_error("Native ray fallback changed filtering");
    scope->fullRay = scope->composed = true;
    if (!rt_spot_runtime::NeedsNativeShadowFiltering()) throw std::runtime_error("Full-distance shadows lost native filtering");
    passForwarded = true;
}
static void FakeIncompleteSpotPass(void*, void*, void*, void*) {
    const auto* scope = native_lighting::rt_spot_runtime::active;
    if (!scope || scope->requested)
        throw std::runtime_error("Incomplete spatial shaders enabled RT replacement");
}
static void TestSpotPassSelection() {
    using namespace native_lighting;
    spot_culling_runtime::ready.store(false); // R78 does not require expansion hooks.
    // A partial R71 install must retain native rendering, even if UI requested RT.
    readyMask.fetch_or(kRtSpotReadyBit);
    SelectMode(Mode::RtSpot);
    rt_spot_runtime::originalPass = &FakeIncompleteSpotPass;
    for (unsigned missing : {rt_spot_runtime::kSpatialComponents[0],rt_spot_runtime::kSpatialComponents[1],rt_spot_runtime::kSelectorComponent}) {
        rt_spot_runtime::components.store(rt_spot_runtime::kRequiredComponents & ~missing);
        rt_spot_runtime::HookPass(nullptr,nullptr,nullptr,nullptr);
    }
    rt_spot_runtime::components.store(rt_spot_runtime::kRequiredComponents);
    readyMask.fetch_or(kRtSpotReadyBit);
    SelectMode(Mode::RtSpot);
    rt_spot_runtime::originalPass = &FakeSpotPass;
    rt_spot_runtime::HookPass(&passArguments[0], &passArguments[1], &passArguments[2], &passArguments[3]);
    if (!passForwarded || rt_spot_runtime::active || rt_spot_runtime::NeedsNativeShadowFiltering() ||
        LiveSelection::GetMode(selection.Read()) != Mode::Native || LiveSelection::Observed(selection.Read()) != 0)
        throw std::runtime_error("RT pass restored incorrectly or marked a newer selection");
    rt_spot_runtime::components.store(0);
    spot_culling_runtime::ready.store(false);
    rt_spot_runtime::previousFull = false;
    readyMask.fetch_and(~kRtSpotReadyBit);
    std::cout << "PASS: RT pass forwards arguments, holds selection and rejects stale observation\n";
}
int main() try {
    TestSpotCulling();
    TestSpotPassSelection();
    using namespace native_lighting;
    TestAllocatorForwarding();
    ComPtr<IDXGIFactory4> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter; Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device; Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    InstallLiveDiagnostic(device.Get());
    D3D12_ROOT_PARAMETER parameter{}; parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV; parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};rootDesc.NumParameters=1;rootDesc.pParameters=&parameter;
    ComPtr<ID3DBlob> serialized,errors;Check(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors));
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)));
    std::vector<unsigned char> lastShader;
    auto makePso=[&](unsigned value){
        const std::string code="RWStructuredBuffer<uint> output : register(u0); [numthreads(1,1,1)] void main(){output[0]="+std::to_string(value)+";}";
        ComPtr<ID3DBlob> shader;Check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"main","cs_5_0",0,0,&shader,&errors));
        lastShader.assign(static_cast<const unsigned char*>(shader->GetBufferPointer()), static_cast<const unsigned char*>(shader->GetBufferPointer()) + shader->GetBufferSize());
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();desc.CS={shader->GetBufferPointer(),shader->GetBufferSize()};
        ID3D12PipelineState* pso=nullptr;Check(device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&pso)));return pso;
    };
    {
        namespace rt = rt_spot_runtime;
        ComPtr<ID3D12PipelineState> owner, filtered, raw, reset;
        owner.Attach(makePso(101)); filtered.Attach(makePso(102));
        raw.Attach(makePso(103)); reset.Attach(makePso(104));
        Check(owner->SetPrivateDataInterface(rt::kVariant,reset.Get()));
        Check(owner->SetPrivateDataInterface(rt::kHistoryVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kRawVariant,raw.Get()));
        Check(owner->SetPrivateDataInterface(rt::kVisibilityVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kLightIdsVariant,raw.Get()));
        Check(owner->SetPrivateDataInterface(rt::kSelectedOffVariant,filtered.Get()));
        rt::Pass pass; pass.raySeen=pass.fullRay=true; rt::active=&pass;
        for (auto mode : {Mode::RtSpot,Mode::RtSpotRaw,Mode::RtSpotMapGuard,Mode::RtSpotNativeCull,Mode::RtSpotVisibilityRaw,Mode::RtSpotVisibilityFiltered,Mode::RtSpotLightIds,Mode::RtSpotBaseline,Mode::RtSpotDiffuseFiltered,Mode::RtSpotGiFiltered}) {
            if (!IsRtSpotMode(mode) || Disables(mode,kDirectBit) || RequiredVariant(mode)!=kRtSpotReadyBit)
                throw std::runtime_error("Diagnostic mode policy mismatch");
            pass.mode=mode;
            for (unsigned kind : {rt::kResetComponent,rt::kSpatialComponents[0],rt::kLightComponent}) {
                Check(owner->SetPrivateData(rt::kKind,sizeof(kind),&kind));
                auto* expected=kind==rt::kResetComponent ? filtered.Get() : reset.Get();
                if (UsesRawSpotVisibility(mode) && kind!=rt::kLightComponent) expected=raw.Get();
                if (mode==Mode::RtSpotMapGuard && kind==rt::kLightComponent) expected=filtered.Get();
                if (kind==rt::kLightComponent && ShowsSpotVisibility(mode)) expected=filtered.Get();
                if (kind==rt::kLightComponent && mode==Mode::RtSpotLightIds) expected=raw.Get();
                auto* actual=rt::SelectCompute(owner.Get());
                if(actual!=expected) throw std::runtime_error("Diagnostic pipeline route mismatch");
                actual->Release();
            }
        }
        pass.fullRay=false;pass.reset=false;
        if(rt::SelectCompute(owner.Get())) throw std::runtime_error("Diagnostic leaked outside full ray pass");
        rt::active=nullptr;
        if (!UsesUnshadowedSpotSelection(Mode::RtSpotBaseline) || !UsesUnshadowedSpotSelection(Mode::RtSpot))
            throw std::runtime_error("R94 comparison must preserve the same selector");
        if (rt::RayRecordOffset(Mode::RtSpotBaseline) != 64 || rt::RayRecordOffset(Mode::RtSpot) != 0 ||
            rt::RayRecordOffset(Mode::RtSpotGiFiltered) != 0 || rt::RayRecordOffset(Mode::RtSpotDiffuseFiltered) != 0 || rt::RayRecordOffset(Mode::RtSpotRaw) != 0 || rt::kRayRecordBufferSize != 128)
            throw std::runtime_error("Live ray record selection or alignment mismatch");
        const unsigned producer=rt::kSelectorComponent;
        Check(owner->SetPrivateData(rt::kKind,sizeof(producer),&producer));
        rt::scopeReady=true;rt::components.store(rt::kRequiredComponents);rt::disabled.store(false);
        for (auto mode : {Mode::Native,Mode::RtSpotBaseline,Mode::RtSpot,Mode::RtSpotVisibilityRaw,Mode::RtSpotLightIds,Mode::RtSpotDiffuseFiltered,Mode::RtSpotGiFiltered}) {
            selection.Select(mode);
            auto* actual=rt::SelectCompute(owner.Get());
            auto* expected=UsesUnshadowedSpotSelection(mode)?reset.Get():nullptr;
            if(actual!=expected)throw std::runtime_error("Pre-ray selector routing mismatch");
            if(actual)actual->Release();
        }
        rt::components.store(rt::kRequiredComponents & ~rt::kSelectorComponent);
        if(rt::SelectCompute(owner.Get()))throw std::runtime_error("Partial selector preparation escaped fallback");
        rt::components.store(0);rt::scopeReady=false;selection.Select(Mode::Native);
        std::cout << "PASS diagnostic PSO routes, point-independent mode policy and fallback scope\n";
    }
    ComPtr<ID3D12PipelineState> direct,gi;direct.Attach(makePso(11));gi.Attach(makePso(22));
    AttachVariant(direct.Get(),makePso(0),kDirectBit,S_OK);AttachVariant(gi.Get(),makePso(0),kGiBit,S_OK);
    AttachVariant(direct.Get(),makePso(12),kDirectBit,S_OK,kDiffuseVariantGuid,kDiffuseReadyBit);
    AttachVariant(direct.Get(),makePso(13),kDirectBit,S_OK,kSpecularVariantGuid,kSpecularReadyBit);
    AttachVariant(direct.Get(),makePso(14),kDirectBit,S_OK,kShadowVariantGuid,kShadowReadyBit);
    AttachVariant(direct.Get(),makePso(16),kDirectBit,S_OK,kScreenVariantGuid,kScreenReadyBit);
    AttachVariant(direct.Get(),makePso(17),kDirectBit,S_OK,kSpotVariantGuid,kSpotReadyBit);
    AttachVariant(direct.Get(),makePso(18),kDirectBit,S_OK,kPointVariantGuid,kPointReadyBit);
    AttachVariant(direct.Get(),makePso(19),kDirectBit,S_OK,kSpotVarianceLowVariantGuid,kSpotVarianceLowReadyBit);
    AttachVariant(direct.Get(),makePso(20),kDirectBit,S_OK,kSpotVarianceHighVariantGuid,kSpotVarianceHighReadyBit);
    AttachVariant(direct.Get(),makePso(21),kDirectBit,S_OK,kSpotPcfCenterVariantGuid,kSpotPcfCenterReadyBit);
    AttachVariant(direct.Get(),makePso(23),kDirectBit,S_OK,kSpotPcf32VariantGuid,kSpotPcf32ReadyBit);
    AttachVariant(direct.Get(),makePso(24),kDirectBit,S_OK,kSpotStableSoftVariantGuid,kSpotStableSoftReadyBit);
    AttachVariant(direct.Get(),makePso(25),kDirectBit,S_OK,kSpotStableTightVariantGuid,kSpotStableTightReadyBit);
    AttachVariant(direct.Get(),makePso(26),kDirectBit,S_OK,kSpotBilinearVariantGuid,kSpotBilinearReadyBit);
    AttachVariant(direct.Get(),makePso(15),kDirectBit,S_OK,kPatternVariantGuid,kPatternReadyBit);
    D3D12_COMMAND_QUEUE_DESC queueDesc{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=256;buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;buffer.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_HEAP_PROPERTIES heap{};heap.CreationNodeMask=1;heap.VisibleNodeMask=1;heap.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> gpu,readback;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&gpu)));
    heap.Type=D3D12_HEAP_TYPE_READBACK;buffer.Flags=D3D12_RESOURCE_FLAG_NONE;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));UINT64 serial=0;
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("event failed");
    auto render=[&](ID3D12PipelineState* native,unsigned expected,bool useReset){
        Check(allocator->Reset());Check(list->Reset(allocator.Get(),useReset?native:nullptr));
        if(!useReset)list->SetPipelineState(native);
        list->SetComputeRootSignature(root.Get());list->SetComputeRootUnorderedAccessView(0,gpu->GetGPUVirtualAddress());list->Dispatch(1,1,1);
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={gpu.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
        list->CopyBufferRegion(readback.Get(),0,gpu.Get(),0,4);
        std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);Check(list->Close());
        ID3D12CommandList* commands[]={list.Get()};queue->ExecuteCommandLists(1,commands);Check(queue->Signal(fence.Get(),++serial));Check(fence->SetEventOnCompletion(serial,event));
        if(WaitForSingleObject(event,30000)!=WAIT_OBJECT_0)throw std::runtime_error("GPU wait failed");
        void* data=nullptr;D3D12_RANGE range{0,4};Check(readback->Map(0,&range,&data));unsigned actual=*static_cast<unsigned*>(data);D3D12_RANGE written{0,0};readback->Unmap(0,&written);
        if(actual!=expected)throw std::runtime_error("wrong GPU result "+std::to_string(actual)+" expected "+std::to_string(expected));
    };
    for(bool reset : {false,true}){
        SelectMode(Mode::Native);render(direct.Get(),11,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::DisableBakedGi);render(direct.Get(),11,reset);render(gi.Get(),0,reset);
        SelectMode(Mode::DisableDirect);render(direct.Get(),0,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::DisableDirectDiffuse);render(direct.Get(),12,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::DisableDirectSpecular);render(direct.Get(),13,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::BypassShadows);render(direct.Get(),14,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::BypassScreenShadows);render(direct.Get(),16,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::BypassSpotShadows);render(direct.Get(),17,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::BypassPointShadows);render(direct.Get(),18,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotVarianceLow);render(direct.Get(),19,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotVarianceHigh);render(direct.Get(),20,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotPcfCenter);render(direct.Get(),21,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotPcf32);render(direct.Get(),23,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotStableSoft);render(direct.Get(),24,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotStableTight);render(direct.Get(),25,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotBilinear);render(direct.Get(),26,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::SpotResolution2x);render(direct.Get(),11,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::DisableLightPattern);render(direct.Get(),15,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::Native);render(direct.Get(),11,reset);render(gi.Get(),22,reset);
    }
    // Reload a serialized native PSO. Its public creation hook need not run;
    // the library load observer must still run, then live routing must work.
    ComPtr<ID3D12Device1> device1; Check(device.As(&device1));
    ComPtr<ID3D12PipelineLibrary> library;
    Check(device1->CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&library)));
    ComPtr<ID3D12PipelineState> cachedSource; cachedSource.Attach(makePso(33));
    const auto cachedShader = lastShader;
    Check(library->StorePipeline(L"cached-direct", cachedSource.Get()));
    std::vector<unsigned char> cache(library->GetSerializedSize());
    Check(library->Serialize(cache.data(), cache.size()));
    cachedSource.Reset(); library.Reset();
    Check(device1->CreatePipelineLibrary(cache.data(), cache.size(), IID_PPV_ARGS(&library)));
    D3D12_COMPUTE_PIPELINE_STATE_DESC cachedDesc{}; cachedDesc.pRootSignature=root.Get();
    cachedDesc.CS={cachedShader.data(),cachedShader.size()};
    ComPtr<ID3D12PipelineState> cached;
    const auto priorLoads=libraryComputeSeen.load(), priorObservations=computeSeen.load();
    Check(library->LoadComputePipeline(L"cached-direct", &cachedDesc, IID_PPV_ARGS(&cached)));
    if(libraryComputeSeen.load()!=priorLoads+1 || computeSeen.load()<=priorObservations)
        throw std::runtime_error("cached load bypassed diagnostic observer");
    AttachVariant(cached.Get(),makePso(0),kDirectBit,S_OK);
    SelectMode(Mode::Native);render(cached.Get(),33,false);
    SelectMode(Mode::DisableDirect);render(cached.Get(),0,false);
    SelectMode(Mode::Native);render(cached.Get(),33,true);
    std::cout<<"PASS: serialized pipeline-library load observed and live native/off/native GPU output verified\n";
    CloseHandle(event);std::cout<<"PASS: WARP live native/GI/direct switching through SetPipelineState and Reset; native restored without restart\n";return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
