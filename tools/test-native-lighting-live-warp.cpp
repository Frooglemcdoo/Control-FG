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
#include <thread>
#include <future>
#include <chrono>
static std::atomic<unsigned long long> presentCount{0};
static void Log(const char*, ...) {}
static bool PTReflectionSha256(const unsigned char*, size_t, char*) { return false; }
// Renderer and sidecar operations are unavailable in the standalone WARP test.
static HMODULE verifiedRenderer = nullptr;
static HMODULE verifiedD3d = nullptr;
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

// The audit uses only QueryInterface on the state; absence of properties must
// not prevent bounded upload sampling or accidentally dispatch any GPU work.
class AuditState final : public ID3D12StateObject {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void** out) override { if(!out)return E_POINTER;*out=nullptr;return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID,UINT*,void*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID,UINT,const void*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,const IUnknown*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetName(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID,void**) override { return E_NOTIMPL; }
};
static void TestHitSetupAudit(ID3D12Device* device) {
    namespace audit=native_lighting::rt_spot_capture_audit;
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameter.Constants={0,3,10};
    D3D12_ROOT_SIGNATURE_DESC rootDesc{1,&parameter,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> serialized,error;
    Check(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)));
    audit::RememberRoot(root.Get(),serialized->GetBufferPointer(),serialized->GetBufferSize());
    audit::DescribeRoot(root.Get(),false);
    std::vector<char> log(native_lighting::DiagnosticLogQueue::Capacity());
    const auto length=native_lighting::diagnosticLog.Drain(log.data(),log.size());
    if(std::string(log.data(),length).find("rt_hit_setup_constants local=0 parameter=0 count=10 reg=0 space=3")==std::string::npos)
        throw std::runtime_error("dynamic root decoder did not report native constants");
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width=65536;desc.Height=desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> resource;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&resource)));
    void* mapped=nullptr;D3D12_RANGE empty{0,0};Check(resource->Map(0,&empty,&mapped));
    unsigned char bytes[384];for(unsigned i=0;i<sizeof(bytes);++i)bytes[i]=static_cast<unsigned char>(i*17);
    std::memcpy(static_cast<unsigned char*>(mapped)+128,bytes,sizeof(bytes));
    D3D12_RANGE written{128,128+sizeof(bytes)};resource->Unmap(0,&written);
    audit::ObserveOwner(resource.Get(),D3D12_RESOURCE_STATE_COMMON);
    if(audit::retainedBytes)throw std::runtime_error("non-generic-read owner was retained");
    // Reproduce R118: unrelated uploads fill every speculative slot before the
    // shader-table allocation appears. Exact address tracking must still find it.
    ComPtr<ID3D12Resource> unrelated[audit::kMaximumOwners];
    for (auto& other : unrelated) {
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&other)));
        audit::ObserveOwner(other.Get(),D3D12_RESOURCE_STATE_GENERIC_READ);
    }
    const auto retained = audit::retainedBytes;
    audit::ObserveOwner(unrelated[0].Get(),D3D12_RESOURCE_STATE_GENERIC_READ);
    if(audit::retainedBytes!=retained)throw std::runtime_error("duplicate upload owner retained");
    audit::ObserveOwner(resource.Get(),D3D12_RESOURCE_STATE_GENERIC_READ);
    if(audit::capacityRejects.load()!=1)throw std::runtime_error("saturated cache did not report capacity rejection");
    D3D12_DISPATCH_RAYS_DESC rays{};
    const auto address=resource->GetGPUVirtualAddress();
    rays.HitGroupTable={address+128,sizeof(bytes),96};
    AuditState state;audit::InspectTable(&state,&rays);
    if(audit::captured.load())throw std::runtime_error("unobserved owner unexpectedly captured");
    unrelated[0]->GetGPUVirtualAddress();
    if(audit::targetedOwner.resource)throw std::runtime_error("unrelated address claimed target");
    if(resource->GetGPUVirtualAddress()!=address)throw std::runtime_error("address hook changed native return value");
    if(audit::targetedOwner.resource.Get()!=resource.Get())throw std::runtime_error("targeted address observer missed saturated-cache owner");
    audit::InspectTable(&state,&rays);
    if(audit::attempts.load()!=1)throw std::runtime_error("retry interval was not enforced");
    audit::nextAttemptMs.store(0);
    audit::InspectTable(&state,&rays);
    if(!audit::captured.load())throw std::runtime_error("bounded upload audit did not complete");
    D3D12_RANGE read{128,128+sizeof(bytes)};Check(resource->Map(0,&read,&mapped));
    const bool unchanged=std::memcmp(static_cast<unsigned char*>(mapped)+128,bytes,sizeof(bytes))==0;
    resource->Unmap(0,&empty);
    if(!unchanged)throw std::runtime_error("setup audit changed native shader-table bytes");
    audit::ReleaseFinishedOwners();
    if(audit::retainedBytes || audit::targetedOwner.resource)throw std::runtime_error("temporary upload owners not released");
    for(const auto& owner:audit::owners)if(owner.resource)throw std::runtime_error("upload owner retained after completion");
    std::cout<<"PASS setup audit: saturated-cache address recovery, retry throttling, bounded upload sample, unchanged records, no GPU dispatch, owner release\n";
}
static bool captureDescriptionVerified=false;
static HRESULT STDMETHODCALLTYPE InspectCapturePipeline(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC* desc,REFIID,void**) {
    if(desc->NumSubobjects!=9)return E_INVALIDARG;
    const auto& native=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[0].pDesc);
    const auto& config=*static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[4].pDesc);
    const auto& alpha=*static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[3].pDesc);
    const auto& association=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[8].pDesc);
    captureDescriptionVerified=native.NumExports==1 && !wcscmp(native.pExports[0].Name,L"shadowAlphaTestAnyHit") &&
        config.MaxPayloadSizeInBytes==32 && config.MaxAttributeSizeInBytes==8 &&
        !wcscmp(alpha.ClosestHitShaderImport,L"captureClosestHit") && !wcscmp(alpha.AnyHitShaderImport,L"shadowAlphaTestAnyHit") &&
        association.pSubobjectToAssociate==&desc->pSubobjects[6] && association.NumExports==2;
    return E_NOTIMPL; // Exercise driver rejection without requiring DXR on WARP.
}
static void TestCapturePipelineDescription() {
    D3D12_DXIL_LIBRARY_DESC library{};D3D12_GLOBAL_ROOT_SIGNATURE global{};D3D12_LOCAL_ROOT_SIGNATURE local{};
    D3D12_HIT_GROUP_DESC opaque{},alpha{};
    opaque.HitGroupExport=L"HitGroup_0_0";opaque.ClosestHitShaderImport=L"shadowClosestHit";
    alpha=opaque;alpha.HitGroupExport=L"HitGroup_1_0";alpha.AnyHitShaderImport=L"shadowAlphaTestAnyHit";
    D3D12_STATE_SUBOBJECT objects[]={
        {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library},
        {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&global},
        {D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&local},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&opaque},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&alpha}
    };
    D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,5,objects};
    const auto saved=native_lighting::rt_shadow_capture::originalState;
    native_lighting::rt_shadow_capture::originalState=&InspectCapturePipeline;
    const auto result=native_lighting::rt_spot_hit_pipeline::Prepare(nullptr,&desc,nullptr);
    native_lighting::rt_shadow_capture::originalState=saved;
    if(result!=E_NOTIMPL || !captureDescriptionVerified || wcscmp(opaque.ClosestHitShaderImport,L"shadowClosestHit"))
        throw std::runtime_error("isolated capture description or failure handling incorrect");
    std::cout<<"PASS capture pipeline description: native alpha export, independent 32-byte payload, original local root, untouched native descriptor, driver failure returned\n";
}

static void TestCopyAudit(ID3D12Device* device) {
    namespace audit=native_lighting::rt_spot_capture_audit;
    namespace copies=native_lighting::rt_spot_copy_audit;
    namespace mirror=native_lighting::rt_spot_table_audit;
    audit::captured.store(false);audit::attempts.store(0);audit::nextAttemptMs.store(0);
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width=65536;desc.Height=desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> source,destination,readback;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&source)));
    heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&destination)));
    heap.Type=D3D12_HEAP_TYPE_READBACK;
    Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));
    std::vector<unsigned char> expected(65536);for(size_t i=0;i<expected.size();++i)expected[i]=static_cast<unsigned char>(i*13);
    unsigned char ids[2][32]{};ids[0][0]=1;ids[1][0]=2;
    for(unsigned i=0;i<4;++i)std::memcpy(expected.data()+128+i*96,ids[i%2],32);
    D3D12_RANGE empty{0,0};void* data=nullptr;Check(source->Map(0,&empty,&data));
    std::memcpy(data,expected.data(),expected.size());D3D12_RANGE written{0,65536};source->Unmap(0,&written);
    audit::targetAddress=destination->GetGPUVirtualAddress()+128;audit::targetBytes=384;audit::targetActive.store(true);
    copies::Observe(destination.Get(),~UINT64(0),source.Get(),0,384);
    if(copies::matches.load())throw std::runtime_error("overflowing copy was sampled");
    copies::Observe(destination.Get(),64,source.Get(),32,192);
    if(copies::samples.load()!=1)throw std::runtime_error("partial copy evidence missing");
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    copies::InstallList(list.Get());
    mirror::armed.store(false);
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    if(mirror::eventCount || mirror::copiedBytes)throw std::runtime_error("capture ran before user request");
    // Retired upload-cache cleanup must not clear the journal's target.
    audit::retentionEnabled.store(false);audit::captured.store(true);audit::ReleaseFinishedOwners();
    if(!audit::targetActive.load() || !audit::targetAddress)throw std::runtime_error("retired cache erased submission target");
    audit::retentionEnabled.store(true);audit::captured.store(false);
    mirror::Request();mirror::BeginIfRequested();
    ComPtr<ID3D12CommandAllocator> dispatchAllocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&dispatchAllocator)));
    ComPtr<ID3D12GraphicsCommandList> dispatchList;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,dispatchAllocator.Get(),nullptr,IID_PPV_ARGS(&dispatchList)));
    // Record the consumer first: CPU recording order differs from submission.
    unsigned char captureIds[2][32]{};captureIds[0][0]=31;captureIds[1][0]=32;
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids,captureIds);Check(dispatchList->Close());
    list->CopyBufferRegion(destination.Get(),64,source.Get(),32,512);
    list->CopyResource(destination.Get(),source.Get());
    if(copies::samples.load()!=3)throw std::runtime_error("copy hooks did not sample both native copy forms");
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={destination.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE};
    list->ResourceBarrier(1,&barrier);list->CopyResource(readback.Get(),destination.Get());Check(list->Close());
    D3D12_COMMAND_QUEUE_DESC queueDesc{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("copy test event creation failed");
    mirror::InstallQueue(queue.Get());
    ID3D12CommandList* lists[]={list.Get(),dispatchList.Get()};queue->ExecuteCommandLists(2,lists);Check(queue->Signal(fence.Get(),1));Check(fence->SetEventOnCompletion(1,event));
    const auto wait=WaitForSingleObject(event,30000);CloseHandle(event);if(wait!=WAIT_OBJECT_0)throw std::runtime_error("copy test GPU timeout");
    D3D12_RANGE read{0,65536};Check(readback->Map(0,&read,&data));
    const bool unchanged=std::memcmp(data,expected.data(),expected.size())==0;readback->Unmap(0,&empty);
    if(!unchanged)throw std::runtime_error("copy observer changed GPU destination bytes");
    Check(source->Map(0,&read,&data));const bool sourceUnchanged=std::memcmp(data,expected.data(),expected.size())==0;source->Unmap(0,&empty);
    if(!sourceUnchanged)throw std::runtime_error("copy observer changed upload bytes");
    std::vector<char> log(native_lighting::DiagnosticLogQueue::Capacity());const auto length=native_lighting::diagnosticLog.Drain(log.data(),log.size());
    const std::string output(log.data(),length);
    if(output.find("destination_heap=1")==std::string::npos || output.find("full_previous_table=0")==std::string::npos || output.find("dispatch_snapshot=0")==std::string::npos)
        throw std::runtime_error("copy audit omitted heap/partial/snapshot limits");
    if(!mirror::done.load() || output.find("identifiers_validated=1")==std::string::npos)
        throw std::runtime_error("submission order did not join separate producer and consumer lists");
    if(output.find("rt_hit_table_translation ready=1")==std::string::npos)throw std::runtime_error("live table was not translated for its capture pipeline");
    for(unsigned i=0;i<4;++i) {
        if(std::memcmp(mirror::translatedTable+i*96,captureIds[i%2],32) || std::memcmp(mirror::translatedTable+i*96+32,expected.data()+128+i*96+32,64))
            throw std::runtime_error("live translation changed a local argument or used the wrong state identifier");
    }
    // A fresh request must not inherit the previous table or spent budget.
    mirror::Request();mirror::BeginIfRequested();
    if(mirror::copiedBytes || mirror::coverage.count)throw std::runtime_error("rearm inherited old capture data");
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    ID3D12CommandList* reversed[]={dispatchList.Get(),list.Get()};mirror::Submitted(queue.Get(),2,reversed);
    if(mirror::done.load())throw std::runtime_error("consumer-before-producer was falsely validated");
    ID3D12CommandList* consumer[]={dispatchList.Get()};mirror::Submitted(queue.Get(),1,consumer);
    if(!mirror::done.load())throw std::runtime_error("command-list replay was accepted");
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    const auto oldToken=mirror::Token(list.Get());Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));
    if(mirror::Token(list.Get())==oldToken)throw std::runtime_error("reset retained generation");
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(queue.Get(),2,lists);
    if(mirror::done.load() || mirror::coverage.count)throw std::runtime_error("unsubmitted pre-reset copies were reused");
    Check(list->Close());
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    ID3D12CommandList* producer[]={list.Get()};mirror::Submitted(queue.Get(),1,producer);
    ComPtr<ID3D12CommandQueue> otherQueue;Check(device->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&otherQueue)));
    mirror::Submitted(otherQueue.Get(),1,consumer);
    if(mirror::done.load())throw std::runtime_error("unverified cross-queue dispatch was validated or aborted instead of deferred");
    // A later correctly fenced dispatch can validate without restarting capture.
    ComPtr<ID3D12Fence> orderingFence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&orderingFence)));
    const auto fenceOperation=[&](ID3D12CommandQueue* q,mirror::QueueOperation op,ID3D12Fence* f,UINT64 value,HRESULT result=S_OK) {
        mirror::CaptureLease lease;if(!lease)throw std::runtime_error("fence test lease unavailable");
        mirror::EndQueueCall(mirror::BeginQueueCall(q,op,0,nullptr,f,value),result);
    };
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),1);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,orderingFence.Get(),1);
    mirror::Token(dispatchList.Get(),true);mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(otherQueue.Get(),1,consumer);
    if(!mirror::validated.load())throw std::runtime_error("fenced cross-queue table failed validation");
    // Real native queue hooks: wait and consumer arrive before producer/signal.
    // The queue wait is GPU-side; this CPU thread must continue to the producer.
    mirror::Request();mirror::BeginIfRequested();
    Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));
    barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1,&barrier);list->CopyResource(destination.Get(),source.Get());
    barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(1,&barrier);list->CopyResource(readback.Get(),destination.Get());Check(list->Close());
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::InstallQueue(otherQueue.Get());
    Check(otherQueue->Wait(orderingFence.Get(),2));
    otherQueue->ExecuteCommandLists(1,consumer);
    if(mirror::done.load() || mirror::coverage.count)throw std::runtime_error("future wait consumed unsubmitted data");
    queue->ExecuteCommandLists(1,producer);Check(queue->Signal(orderingFence.Get(),2));
    if(!mirror::validated.load())throw std::runtime_error("future signal did not release deferred consumer");
    ComPtr<ID3D12Fence> completion;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&completion)));
    Check(otherQueue->Signal(completion.Get(),1));
    HANDLE completed=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!completed)throw std::runtime_error("queue completion event failed");
    Check(completion->SetEventOnCompletion(1,completed));const auto completedWait=WaitForSingleObject(completed,30000);CloseHandle(completed);
    if(completedWait!=WAIT_OBJECT_0)throw std::runtime_error("native queue Signal/Wait forwarding stalled");
    Check(readback->Map(0,&read,&data));
    const bool crossQueueUnchanged=std::memcmp(data,expected.data(),expected.size())==0;readback->Unmap(0,&empty);
    if(!crossQueueUnchanged)throw std::runtime_error("cross-queue observer changed GPU bytes");
    // A wait for the wrong value cannot import an earlier producer clock.
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(queue.Get(),1,producer);
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),3);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,orderingFence.Get(),4);
    mirror::Submitted(otherQueue.Get(),1,consumer);
    if(mirror::done.load())throw std::runtime_error("insufficient fence value released consumer");
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),5);
    if(!mirror::validated.load())throw std::runtime_error("greater fence value failed to release consumer");
    // Three queues must preserve transitive signal/wait dependencies.
    ComPtr<ID3D12CommandQueue> thirdQueue;Check(device->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&thirdQueue)));
    ComPtr<ID3D12Fence> relayFence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&relayFence)));
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(queue.Get(),1,producer);
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),6);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,orderingFence.Get(),6);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Signal,relayFence.Get(),1);
    fenceOperation(thirdQueue.Get(),mirror::QueueOperation::Wait,relayFence.Get(),1);
    mirror::Submitted(thirdQueue.Get(),1,consumer);
    if(!mirror::validated.load())throw std::runtime_error("transitive queue dependency was lost");
    // Failed calls and ambiguous fence reuse must never become valid edges.
    mirror::Request();mirror::BeginIfRequested();
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),7,E_FAIL);
    if(!mirror::done.load() || mirror::validated.load())throw std::runtime_error("failed signal accepted");
    mirror::Request();mirror::BeginIfRequested();
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),7);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),8);
    if(mirror::done.load() || mirror::validated.load() || !mirror::fences[mirror::FindFence(orderingFence.Get())].unsupported)throw std::runtime_error("multiple fence producers were not isolated");
    mirror::Request();mirror::BeginIfRequested();
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),7);
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),6);
    if(mirror::done.load() || mirror::validated.load() || !mirror::fences[mirror::FindFence(orderingFence.Get())].unsupported)throw std::runtime_error("fence rewind was not isolated");
    // Metadata calls on different queues may overlap. Publish the wait before
    // the in-flight signal completes, then verify deferred progress.
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(queue.Get(),1,producer);
    const auto pendingSignal=mirror::BeginQueueCall(queue.Get(),mirror::QueueOperation::Signal,0,nullptr,orderingFence.Get(),9);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,orderingFence.Get(),9);
    mirror::Submitted(otherQueue.Get(),1,consumer);
    if(mirror::done.load())throw std::runtime_error("unfinished native signal published early");
    mirror::EndQueueCall(pendingSignal);
    if(!mirror::validated.load())throw std::runtime_error("overlapping different queues did not resolve");
    mirror::Request();mirror::BeginIfRequested();
    mirror::InstallFence(orderingFence.Get());Check(orderingFence->Signal(10));
    if(mirror::done.load() || mirror::validated.load() || orderingFence->GetCompletedValue()!=10 || !mirror::fences[mirror::FindFence(orderingFence.Get())].unsupported)
        throw std::runtime_error("CPU fence signal was not forwarded and rejected as GPU evidence");
    // Repeated values cannot prove work added after the first sufficient signal.
    mirror::Request();mirror::BeginIfRequested();
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,relayFence.Get(),1);
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    mirror::Submitted(queue.Get(),1,producer);
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,relayFence.Get(),1);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,relayFence.Get(),1);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(otherQueue.Get(),1,consumer);
    if(mirror::done.load())throw std::runtime_error("repeated signal either aborted or imported later work");
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,relayFence.Get(),2);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,relayFence.Get(),2);
    mirror::Token(dispatchList.Get(),true);mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(otherQueue.Get(),1,consumer);
    if(!mirror::validated.load())throw std::runtime_error("new sufficient signal failed after repeated value");
    // An unrelated ambiguous fence must not destroy independent capture work.
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);mirror::Submitted(queue.Get(),1,producer);
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),11);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),12);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,orderingFence.Get(),12);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);mirror::Submitted(otherQueue.Get(),1,consumer);
    if(mirror::done.load() || !mirror::coverage.Covers(128,384))throw std::runtime_error("isolated fence imported work or destroyed unrelated coverage");
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,relayFence.Get(),1);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,relayFence.Get(),1);
    mirror::Token(dispatchList.Get(),true);mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(otherQueue.Get(),1,consumer);
    if(!mirror::validated.load())throw std::runtime_error("independent dependency failed after fence isolation");
    // Invalidate an already imported edge, including its transitive relay.
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);mirror::Submitted(queue.Get(),1,producer);
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),13);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Wait,orderingFence.Get(),13);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Signal,relayFence.Get(),1);
    fenceOperation(thirdQueue.Get(),mirror::QueueOperation::Wait,relayFence.Get(),1);
    fenceOperation(otherQueue.Get(),mirror::QueueOperation::Signal,orderingFence.Get(),14);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);mirror::Submitted(thirdQueue.Get(),1,consumer);
    if(mirror::done.load())throw std::runtime_error("revoked transitive dependency still validated or aborted capture");
    ComPtr<ID3D12Fence> independentFence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&independentFence)));
    fenceOperation(queue.Get(),mirror::QueueOperation::Signal,independentFence.Get(),1);
    fenceOperation(thirdQueue.Get(),mirror::QueueOperation::Wait,independentFence.Get(),1);
    mirror::Token(dispatchList.Get(),true);mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(thirdQueue.Get(),1,consumer);
    if(!mirror::validated.load())throw std::runtime_error("capture did not recover after dependency revocation");
    // CPU signaling is forwarded and isolated, while unrelated same-queue data survives.
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);mirror::Submitted(queue.Get(),1,producer);
    Check(orderingFence->Signal(20));
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);mirror::Submitted(queue.Get(),1,consumer);
    if(!mirror::validated.load() || orderingFence->GetCompletedValue()!=20)throw std::runtime_error("unrelated CPU signal stopped capture or was not forwarded");
    const auto fenceLogLength=native_lighting::diagnosticLog.Drain(log.data(),log.size());
    const std::string fenceLog(log.data(),fenceLogLength);
    for(const char* marker:{"reason=multiple_queue_producers","reason=decreasing_value","reason=cpu_signal","previous_queue=","previous_value=","baseline=","capture_continues=1","repeated=1"})
        if(fenceLog.find(marker)==std::string::npos)throw std::runtime_error("fence diagnostic detail missing");
    std::cout<<"PASS scoped fence capture: repeated values do not import later work, unrelated ambiguity survives, transitive proofs revoked, independent recovery, native CPU signal forwarding, exact diagnostic values\n";
    std::cout<<"PASS cross-queue capture: native future Signal/Wait forwarding, deferred unordered dispatch, exact and greater values, transitive dependencies, concurrent queues, failed signals and scoped fence isolation\n";
    // Reproduce R125's 6641/6644 cycle with actual native queues. Three
    // producer signals are queued before arming, but held behind a test gate.
    // Their CompletedValue is therefore still 6640 when the observer starts.
    mirror::done.store(true);
    ComPtr<ID3D12Fence> boundaryA,boundaryB,boundaryGate,boundaryComplete;
    Check(device->CreateFence(6640,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&boundaryA)));
    Check(device->CreateFence(6640,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&boundaryB)));
    Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&boundaryGate)));
    Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&boundaryComplete)));
    Check(queue->Wait(boundaryGate.Get(),1));
    Check(queue->Signal(boundaryA.Get(),6641));
    Check(queue->Signal(boundaryA.Get(),6642));
    Check(queue->Signal(boundaryA.Get(),6643));
    mirror::Request();mirror::BeginIfRequested();
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,512);
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    Check(queue->Wait(boundaryB.Get(),6644));
    queue->ExecuteCommandLists(1,consumer);
    Check(queue->Signal(boundaryA.Get(),6644));
    Check(otherQueue->Signal(boundaryB.Get(),6641));
    Check(otherQueue->Signal(boundaryB.Get(),6642));
    Check(otherQueue->Wait(boundaryA.Get(),6641));
    otherQueue->ExecuteCommandLists(1,producer);
    Check(otherQueue->Signal(boundaryB.Get(),6643));
    Check(otherQueue->Signal(boundaryB.Get(),6644));
    const bool prematureBoundary=mirror::done.load() || mirror::coverage.count;
    // Always release the test gate before reporting a failure. This is a test
    // fixture only; the diagnostic never creates or waits on a GPU fence.
    Check(boundaryGate->Signal(1));
    Check(queue->Signal(boundaryComplete.Get(),1));
    HANDLE boundaryEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!boundaryEvent)throw std::runtime_error("boundary test event failed");
    Check(boundaryComplete->SetEventOnCompletion(1,boundaryEvent));
    const auto boundaryWait=WaitForSingleObject(boundaryEvent,30000);CloseHandle(boundaryEvent);
    if(boundaryWait!=WAIT_OBJECT_0)throw std::runtime_error("native boundary cycle did not progress");
    if(prematureBoundary)throw std::runtime_error("boundary wait was released before native completion");
    for(unsigned retry=0;retry<50 && !mirror::done.load();++retry) {
        Sleep(2); // Test-only: exercise the nonblocking 2 ms polling throttle.
        mirror::CaptureLease lease;if(!lease)break;
        AcquireSRWLockExclusive(&mirror::submissionLock);mirror::DrainQueueOrder();ReleaseSRWLockExclusive(&mirror::submissionLock);
    }
    if(!mirror::validated.load() || !mirror::coverage.Covers(128,384))throw std::runtime_error("R125 initial signal gap still trapped replay");
    Check(readback->Map(0,&read,&data));
    const bool boundaryBytes=std::memcmp(data,expected.data(),expected.size())==0;readback->Unmap(0,&empty);
    if(!boundaryBytes)throw std::runtime_error("boundary capture changed native GPU copy results");
    const auto boundaryLogLength=native_lighting::diagnosticLog.Drain(log.data(),log.size());
    const std::string boundaryLog(log.data(),boundaryLogLength);
    if(boundaryLog.find("rt_hit_table_start_boundary ")==std::string::npos || boundaryLog.find("wait=6641")==std::string::npos || boundaryLog.find("first_signal=6644")==std::string::npos || boundaryLog.find("imported_dependency=0 coverage_discarded=1")==std::string::npos)
        throw std::runtime_error("boundary cut evidence missing");
    std::cout<<"PASS capture start boundary: real pre-arm queued signals, 6641/6644 cycle, no early release, no invented dependency, fresh complete table and unchanged GPU bytes\n";
    mirror::Request();mirror::BeginIfRequested();mirror::copiedBytes=mirror::kCopyBudget;
    mirror::Observe(list.Get(),destination.Get(),0,source.Get(),0,192);
    if(!mirror::done.load())throw std::runtime_error("copy budget not enforced");
    // Hold one producer mid-publication while another recording callback and
    // submission run. R122 used an exclusive journal lock across this window.
    mirror::Request();mirror::BeginIfRequested();
    std::promise<void> producerEntered,resumeProducer;
    auto entered=producerEntered.get_future();auto resume=resumeProducer.get_future();
    std::exception_ptr producerError;
    std::thread producerThread([&] {
        try {
            mirror::CaptureLease lease;
            if(!lease)throw std::runtime_error("producer lease unavailable");
            unsigned index=0;UINT64 offset=0;
            if(!mirror::ReserveEvent(index) || !mirror::ReserveBytes(512,offset))throw std::runtime_error("producer reservation failed");
            auto& e=mirror::events[index];e={};e.kind=mirror::Kind::Copy;e.generation=mirror::Token(list.Get());
            e.resource=mirror::Identity(destination.Get(),mirror::kResourceIdentity);e.base=destination->GetGPUVirtualAddress();e.extent=65536;e.size=512;e.archiveOffset=offset;
            std::memcpy(mirror::archive+offset,expected.data(),512);
            producerEntered.set_value();resume.wait();
            mirror::published[index].store(true,std::memory_order_release);
        } catch(...) {producerError=std::current_exception();try{producerEntered.set_value();}catch(...){} }
    });
    entered.wait();
    auto concurrent=std::async(std::launch::async,[&] {
        mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
        mirror::Submitted(queue.Get(),1,consumer);
    });
    const bool progressed=concurrent.wait_for(std::chrono::seconds(2))==std::future_status::ready;
    resumeProducer.set_value();producerThread.join();concurrent.get();
    if(producerError)std::rethrow_exception(producerError);
    if(!progressed || mirror::done.load() || mirror::coverage.count)
        throw std::runtime_error("recording/submission blocked, aborted, or read an unpublished copy");
    mirror::Submitted(queue.Get(),1,producer);
    mirror::Token(dispatchList.Get(),true); // New recording generation for the consumer.
    mirror::RecordDispatch(dispatchList.Get(),audit::targetAddress,384,ids);
    mirror::Submitted(queue.Get(),1,consumer);
    if(!mirror::done.load() || !mirror::coverage.Covers(128,384))throw std::runtime_error("published concurrent copy did not validate");
    mirror::Request();mirror::BeginIfRequested();
    unsigned reservedIndices[512]{};UINT64 reservedOffsets[512]{};
    std::future<void> writers[8];
    for(unsigned t=0;t<8;++t)writers[t]=std::async(std::launch::async,[&,t] {
        mirror::CaptureLease lease;if(!lease)throw std::runtime_error("writer lease unavailable");
        for(unsigned i=0;i<64;++i) {
            const unsigned result=t*64+i;
            if(!mirror::ReserveEvent(reservedIndices[result]) || !mirror::ReserveBytes(32,reservedOffsets[result]))throw std::runtime_error("parallel reservation failed");
        }
    });
    for(auto& writer:writers)writer.get();
    bool usedIndices[512]{},usedOffsets[512]{};
    for(unsigned i=0;i<512;++i) {
        if(reservedIndices[i]>=512 || reservedOffsets[i]%32 || reservedOffsets[i]/32>=512 || usedIndices[reservedIndices[i]] || usedOffsets[reservedOffsets[i]/32])
            throw std::runtime_error("parallel writers overlap journal storage");
        usedIndices[reservedIndices[i]]=true;usedOffsets[reservedOffsets[i]/32]=true;
    }
    mirror::done.store(true);
    std::cout<<"PASS concurrent capture: producer paused before publication, independent recording/submission progress, complete published replay, eight writers without overlapping storage\n";
    std::cout<<"PASS submission capture: idle before request, separate lists in GPU submission order, rearm, reversed order, replay, reset and cross-queue dependencies\n";
    audit::captured.store(true);audit::ReleaseFinishedOwners();
    std::cout<<"PASS copy audit: default-heap destination, partial coverage, overflow rejection, native buffer/resource forwarding and unchanged GPU/upload bytes\n";
}

#include "rt_spot_capture_gpu_warp_test.h"
#include "rt_spot_capture_signal_test.h"

int main() try {
    TestCapturePipelineDescription();
    TestSpotCulling();
    TestSpotPassSelection();
    using namespace native_lighting;
    TestAllocatorForwarding();
    ComPtr<IDXGIFactory4> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter; Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device; Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    TestGpuHitTableTranslation(device.Get());
    TestCompositionSignal(device.Get());
    TestHitCaptureSubmissionPolicy(device.Get());
    TestHitCaptureExportRetry();
    TestHitSetupAudit(device.Get());
    TestCopyAudit(device.Get());
    TestGpuCapturePolicy(device.Get());
    InstallLiveDiagnostic(device.Get());
    TestGpuCapturePolicy(device.Get());
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
        ComPtr<ID3D12PipelineState> owner, filtered, raw, reset, signal;
        owner.Attach(makePso(101)); filtered.Attach(makePso(102));
        raw.Attach(makePso(103)); reset.Attach(makePso(104)); signal.Attach(makePso(105));
        Check(owner->SetPrivateDataInterface(rt::kVariant,reset.Get()));
        Check(owner->SetPrivateDataInterface(rt::kHistoryVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kGuardedHistoryVariant,raw.Get()));
        Check(owner->SetPrivateDataInterface(rt::kRawVariant,raw.Get()));
        Check(owner->SetPrivateDataInterface(rt::kVisibilityVariant,signal.Get()));
        Check(owner->SetPrivateDataInterface(rt::kLightIdsVariant,raw.Get()));
        Check(owner->SetPrivateDataInterface(rt::kSelectedOffVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kOpenVariant,raw.Get()));
        Check(owner->SetPrivateDataInterface(rt::kContactOffVariant,raw.Get()));
        Check(owner->SetPrivateDataInterface(rt::kBentNormalVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kCorrectedOpenVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kMapProbeVariant,signal.Get()));
        Check(owner->SetPrivateDataInterface(rt::kMapPriorityVariant,signal.Get()));
        Check(owner->SetPrivateDataInterface(rt::kCorrectedNoAOVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kLocalMapsVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kScreenOcclusionVariant,filtered.Get()));
        Check(owner->SetPrivateDataInterface(rt::kBandColorsVariant,filtered.Get()));
        rt::Pass pass; pass.raySeen=pass.fullRay=true; rt::active=&pass;
        for (auto mode : {Mode::RtSpot,Mode::RtSpotRaw,Mode::RtSpotMapGuard,Mode::RtSpotNativeCull,Mode::RtSpotVisibilityRaw,Mode::RtSpotVisibilityFiltered,Mode::RtSpotLightIds,Mode::RtSpotBaseline,Mode::RtSpotDiffuseFiltered,Mode::RtSpotGiFiltered,Mode::RtSpotWorldAlbedo,Mode::RtSpotSpecularFiltered,Mode::RtSpotViewAngle,Mode::RtSpotGi2,Mode::RtSpotGi4,Mode::RtSpotGi8,Mode::RtSpotGiMenu,Mode::RtSpotGi12,Mode::RtSpotGi16,Mode::RtSpotSpec1,Mode::RtSpotSpec2,Mode::RtSpotSpec4,Mode::RtSpotSpec8,Mode::RtSpotSpec12,Mode::RtSpotSpec16,Mode::RtSpotTemporal2,Mode::RtSpotTemporal4,Mode::RoomSpot,Mode::RoomBoth,Mode::RoomCorrectedNoBakedGi,Mode::RoomCorrectedNoBakedGiNoRR,Mode::RoomCorrectedVisibility,Mode::RoomCorrectedRawVisibility,Mode::RoomCorrectedLightIds,Mode::RoomCorrectedGuarded,Mode::RoomCorrectedNoAO,Mode::RoomCorrectedNoHistory,Mode::RoomCorrectedRaw,Mode::RoomMapProbe,Mode::RoomProjectionOrigin,Mode::RoomCorrectedMapPriority,Mode::RoomCorrectedOpen,Mode::RoomCorrectedNoRR,Mode::RoomBentNormalOff,Mode::RoomLocalMapsOff,Mode::RoomContactOff,Mode::RoomScreenOcclusionOff,Mode::RoomClipNoRR,Mode::RoomAuthoredClip,Mode::RoomAuthoredSpatial,Mode::RoomAuthoredRaw,Mode::RoomNativeSelection,Mode::RoomOpen,Mode::RoomSelectedOff,Mode::RoomRaw,Mode::RoomBlocker,Mode::RoomHitX,Mode::RoomHitY,Mode::RoomHitZ}) {
            if (!IsRtSpotMode(mode) || Disables(mode,kDirectBit) || RequiredVariant(mode)!=(kRtSpotReadyBit|(DisablesCorrectedBakedGi(mode)?kGiBit:0)))
                throw std::runtime_error("Diagnostic mode policy mismatch");
            pass.mode=mode;
            if(IsCorrectedSignal(mode) && rt::RayRecordOffset(mode)!=384)
                throw std::runtime_error("Signal view lost authored clipping");
            for (unsigned kind : {rt::kResetComponent,rt::kSpatialComponents[0],rt::kLightComponent}) {
                Check(owner->SetPrivateData(rt::kKind,sizeof(kind),&kind));
                auto* expected=kind==rt::kResetComponent ? filtered.Get() : reset.Get();
                if (UsesRawSpotVisibility(mode) && kind!=rt::kLightComponent) expected=raw.Get();
                if (BypassesSpotHistory(mode) && kind==rt::kResetComponent) expected=raw.Get();
                if (UsesGuardedSpotHistory(mode) && kind==rt::kResetComponent) expected=raw.Get();
                if ((mode==Mode::RtSpotMapGuard || mode==Mode::RoomSelectedOff) && kind==rt::kLightComponent) expected=filtered.Get();
                if (kind==rt::kLightComponent && ShowsSpotVisibility(mode)) expected=(mode==Mode::RoomBlocker || IsHitPositionMode(mode))?filtered.Get():signal.Get();
                if (kind==rt::kLightComponent && ShowsSpotLightIds(mode)) expected=raw.Get();
                if ((mode==Mode::RoomOpen || mode==Mode::RoomContactOff) && kind==rt::kLightComponent) expected=raw.Get();
                if (NeedsIsolationProducer(mode) && !IsCorrectedSignal(mode) && kind==rt::kLightComponent) expected=filtered.Get();
                auto* actual=rt::SelectCompute(owner.Get());
                if(actual!=expected) throw std::runtime_error("Diagnostic pipeline route mismatch");
                actual->Release();
            }
        }
        // Guard must not leak into the transition reset or silently route to old history.
        pass.mode=Mode::RoomCorrectedGuarded;pass.reset=true;
        const unsigned temporalKind=rt::kResetComponent;
        Check(owner->SetPrivateData(rt::kKind,sizeof(temporalKind),&temporalKind));
        auto* transition=rt::SelectCompute(owner.Get());
        if(transition!=reset.Get())throw std::runtime_error("Guard bypassed transition reset");
        transition->Release();pass.reset=false;
        Check(owner->SetPrivateDataInterface(rt::kGuardedHistoryVariant,nullptr));
        auto* missingGuard=rt::SelectCompute(owner.Get());
        if(missingGuard){missingGuard->Release();throw std::runtime_error("Missing guard silently fell back");}
        pass.mode=kDefaultMode;
        auto* baselineHistory=rt::SelectCompute(owner.Get());
        if(baselineHistory!=raw.Get())throw std::runtime_error("Optional guard failure broke current-frame baseline");
        baselineHistory->Release();
        pass.fullRay=false;pass.reset=false;
        if(rt::SelectCompute(owner.Get())) throw std::runtime_error("Diagnostic leaked outside full ray pass");
        rt::active=nullptr;
        if (!UsesUnshadowedSpotSelection(Mode::RtSpotBaseline) || !UsesUnshadowedSpotSelection(Mode::RtSpot))
            throw std::runtime_error("R94 comparison must preserve the same selector");
        if (rt::RayRecordOffset(Mode::RtSpotBaseline) != 64 || rt::RayRecordOffset(Mode::RtSpot) != 0 ||
            rt::RayRecordOffset(Mode::RtSpotGi2) != 0 || rt::RayRecordOffset(Mode::RtSpotGiMenu) != 0 || rt::RayRecordOffset(Mode::RtSpotViewAngle) != 0 || rt::RayRecordOffset(Mode::RtSpotSpecularFiltered) != 0 || rt::RayRecordOffset(Mode::RtSpotWorldAlbedo) != 0 || rt::RayRecordOffset(Mode::RtSpotGiFiltered) != 0 || rt::RayRecordOffset(Mode::RtSpotDiffuseFiltered) != 0 || rt::RayRecordOffset(Mode::RtSpotRaw) != 0 || rt::kRayRecordBufferSize != 512 || rt::RayRecordOffset(Mode::RoomBlocker)!=128)
            throw std::runtime_error("Live ray record selection or alignment mismatch");
        if(rt::RayRecordOffset(Mode::RoomProjectionOrigin)!=448 || rt::RayRecordFits(Mode::RoomProjectionOrigin,448) || !rt::RayRecordFits(Mode::RoomProjectionOrigin,512))throw std::runtime_error("Projection record bounds");
        if(rt::RayRecordOffset(Mode::RoomAuthoredClip)!=384 || rt::RayRecordFits(Mode::RoomAuthoredClip,384) || !rt::RayRecordFits(Mode::RoomAuthoredClip,448))throw std::runtime_error("Authored clip record bounds");
        if(rt::RayRecordOffset(Mode::RoomHitX)!=192||rt::RayRecordOffset(Mode::RoomHitY)!=256||rt::RayRecordOffset(Mode::RoomHitZ)!=320||rt::RayRecordFits(Mode::RoomHitX,192)||!rt::RayRecordFits(Mode::RoomHitZ,384))throw std::runtime_error("Hit position record bounds");
        if(rt::RayRecordFits(Mode::RoomBlocker,128)||!rt::RayRecordFits(Mode::RoomBlocker,192)||
           !rt::RayRecordFits(Mode::RoomBoth,128))throw std::runtime_error("Endpoint fallback record bounds");
        const unsigned producer=rt::kSelectorComponent;
        Check(owner->SetPrivateData(rt::kKind,sizeof(producer),&producer));
        rt::scopeReady=true;rt::components.store(rt::kRequiredComponents);rt::disabled.store(false);
        for (auto mode : {Mode::Native,Mode::RtSpotBaseline,Mode::RtSpot,Mode::RtSpotVisibilityRaw,Mode::RtSpotLightIds,Mode::RtSpotDiffuseFiltered,Mode::RtSpotGiFiltered,Mode::RtSpotWorldAlbedo,Mode::RtSpotSpecularFiltered,Mode::RtSpotViewAngle,Mode::RtSpotGi2,Mode::RtSpotGi4,Mode::RtSpotGi8,Mode::RtSpotGiMenu,Mode::RtSpotGi12,Mode::RtSpotGi16,Mode::RtSpotSpec1,Mode::RtSpotSpec2,Mode::RtSpotSpec4,Mode::RtSpotSpec8,Mode::RtSpotSpec12,Mode::RtSpotSpec16,Mode::RtSpotTemporal2,Mode::RtSpotTemporal4,Mode::RoomSpot,Mode::RoomBoth,Mode::RoomCorrectedNoBakedGi,Mode::RoomCorrectedNoBakedGiNoRR,Mode::RoomCorrectedVisibility,Mode::RoomCorrectedRawVisibility,Mode::RoomCorrectedLightIds,Mode::RoomCorrectedGuarded,Mode::RoomCorrectedNoAO,Mode::RoomCorrectedNoHistory,Mode::RoomCorrectedRaw,Mode::RoomMapProbe,Mode::RoomProjectionOrigin,Mode::RoomCorrectedMapPriority,Mode::RoomCorrectedOpen,Mode::RoomCorrectedNoRR,Mode::RoomBentNormalOff,Mode::RoomLocalMapsOff,Mode::RoomContactOff,Mode::RoomScreenOcclusionOff,Mode::RoomClipNoRR,Mode::RoomAuthoredClip,Mode::RoomAuthoredSpatial,Mode::RoomAuthoredRaw,Mode::RoomNativeSelection,Mode::RoomOpen,Mode::RoomSelectedOff,Mode::RoomRaw,Mode::RoomBlocker,Mode::RoomHitX,Mode::RoomHitY,Mode::RoomHitZ}) {
            selection.Select(mode);
            auto* actual=rt::SelectCompute(owner.Get());
            auto* expected=(mode==Mode::RoomMapProbe || mode==Mode::RoomCorrectedMapPriority)?signal.Get():NeedsIsolationProducer(mode)?filtered.Get():UsesUnshadowedSpotSelection(mode)?reset.Get():nullptr;
            if(NeedsIsolationProducer(mode) && LiveSelection::Observed(selection.Read())!=kGiBit)throw std::runtime_error("Missing producer observation");
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
    // New comparisons must actually bind the GI variant, not just observe the
    // corrected producer/composition. Test SetPipelineState and Reset routes.
    for(bool reset : {false,true})for(auto mode:{Mode::RoomCorrectedNoBakedGi,Mode::RoomCorrectedNoBakedGiNoRR}) {
        selection.Select(mode); // Exercise bindings independently of global readiness.
        render(direct.Get(),11,reset);
        if(LiveSelection::Observed(selection.Read())&kBakedGiOverrideBit)
            throw std::runtime_error("Direct binding falsely confirmed baked GI removal");
        render(gi.Get(),0,reset);
        if(!(LiveSelection::Observed(selection.Read())&kBakedGiOverrideBit))
            throw std::runtime_error("Baked GI override was not observed");
        Binding stale(gi.Get());selection.Select(kDefaultMode);stale.Recorded();
        if(LiveSelection::Observed(selection.Read())&kBakedGiOverrideBit)
            throw std::runtime_error("Stale GI binding confirmed a different mode");
        render(gi.Get(),22,reset);
    }
    {
        ComPtr<ID3D12PipelineState> missing;missing.Attach(makePso(22));
        unsigned kind=kGiBit;Check(missing->SetPrivateData(kKindGuid,sizeof(kind),&kind));
        selection.Select(Mode::RoomCorrectedNoBakedGi);
        Binding fallback(missing.Get());fallback.Recorded();
        if(fallback.selected!=missing.Get() || (LiveSelection::Observed(selection.Read())&kBakedGiOverrideBit))
            throw std::runtime_error("Missing baked GI variant falsely reported applied");
        selection.Select(Mode::Native);
    }
    std::cout<<"PASS R147 baked-GI isolation: GPU output, Set/Reset bindings, stale observations, missing variant, baseline restoration\n";
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
    // Repeat F9 must not recycle a recording before both export and list reset.
    {
        namespace capture=rt_spot_capture_runtime;
        if constexpr(rt_spot_capture::GpuCaptureEnabled) {
            capture::ready=true;capture::phase=capture::Phase::Idle;
            capture::signalReady=true;
            if(!capture::RequestFresh() || capture::RequestFresh())throw std::runtime_error("Capture arm must be exclusive");
            capture::CancelPending();
            for(auto phase:{capture::Phase::Recorded,capture::Phase::Submitted,capture::Phase::Failed}){
                capture::phase=phase;if(capture::RequestFresh())throw std::runtime_error("In-flight/failed capture recycled");
            }
            Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(capture::resources.fence.ReleaseAndGetAddressOf())));
            Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(capture::resources.signalFence.ReleaseAndGetAddressOf())));
            Check(list.As(&capture::recordedSignalList));capture::signalGeneration=500;
            Check(list.As(&capture::recordedList));capture::recordedGeneration=500;capture::fenceValue=1;
            UINT64 generation=500;Check(list->SetPrivateData(rt_spot_table_audit::kGeneration,sizeof(generation),&generation));
            capture::phase=capture::Phase::Complete;capture::readbackCopied=true;capture::exportStage=2;
            if(capture::RequestFresh())throw std::runtime_error("Partial export recycled");
            capture::exportStage=3;
            if(capture::RequestFresh())throw std::runtime_error("Unreset command list recycled");
            generation=501;Check(list->SetPrivateData(rt_spot_table_audit::kGeneration,sizeof(generation),&generation));
            if(capture::RequestFresh())throw std::runtime_error("Unfinished fence recycled");
            Check(capture::resources.fence->Signal(1));
            if(capture::RequestFresh())throw std::runtime_error("unfinished composition fence recycled");
            Check(capture::resources.signalFence->Signal(1));
            if(!capture::RequestFresh() || capture::fenceValue!=2 || capture::exportStage!=0 || capture::readbackCopied)
                throw std::runtime_error("Completed reset recording did not rearm cleanly");
            capture::CancelPending();capture::ready=false;
            std::cout<<"PASS F9 rearm: in-flight, failure, partial export, reset and monotonic fence guards\n";
        } else if(capture::RequestFresh())throw std::runtime_error("Capture enabled in disabled build");
    }
    CloseHandle(event);std::cout<<"PASS: WARP live native/GI/direct switching through SetPipelineState and Reset; native restored without restart\n";return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
