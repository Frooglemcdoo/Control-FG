#define NOMINMAX
#include "../src/pt_native_uploads.h"
#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_resource_lease_probe.h"
#include "../src/pt_owned_scene_runtime.h"
#include "../src/pt_scene_submissions.h"
#include "../src/pt_scene_live_owner.h"
#include "../src/pt_scene_queue_runtime.h"
#include "../src/pt_scene_handoff.h"
#include "../src/pt_scene_dispatch.h"
#include "../src/pt_scene_topology_runtime.h"
using Microsoft::WRL::ComPtr;
// COM permits ID3D12Fence and canonical IUnknown to have different addresses.
// This fixture reproduces the interface-key lookup that missed R23 admission.
struct FenceAlias final:ID3D12Fence {
    struct Canonical final:IUnknown {
        FenceAlias* owner=nullptr;
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override{return owner->QueryInterface(iid,out);}
        ULONG STDMETHODCALLTYPE AddRef() override{return 1;}
        ULONG STDMETHODCALLTYPE Release() override{return 1;}
    } canonical;
    FenceAlias(){canonical.owner=this;}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override{
        if(!out)return E_POINTER;*out=nullptr;
        if(iid==__uuidof(IUnknown))*out=static_cast<IUnknown*>(&canonical);
        else if(iid==__uuidof(ID3D12Fence))*out=static_cast<ID3D12Fence*>(this);
        else return E_NOINTERFACE;return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return 1;}
    ULONG STDMETHODCALLTYPE Release() override{return 1;}
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID,UINT*,void*) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID,UINT,const void*) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,const IUnknown*) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetName(LPCWSTR) override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID,void**) override{return E_NOTIMPL;}
    UINT64 STDMETHODCALLTYPE GetCompletedValue() override{return 0;}
    HRESULT STDMETHODCALLTYPE SetEventOnCompletion(UINT64,HANDLE) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE Signal(UINT64) override{return E_NOTIMPL;}
};
static void TestFenceIdentity(PTOwnedScene::Runtime& resources,ID3D12CommandQueue* queue,ID3D12GraphicsCommandList* list){
    FenceAlias alias,other;PTSceneLive::Capture capture(resources);
    assert(static_cast<IUnknown*>(static_cast<ID3D12Fence*>(&alias))!=static_cast<IUnknown*>(&alias.canonical));
    assert(capture.Wait(queue,&alias,2));assert(capture.Mark(list,123456));
    ID3D12CommandList* native=list;assert(capture.Submit(queue,1,&native));
    const auto wait=capture.PendingWait(123456);
    assert(wait.polled&&wait.graph.value==2&&wait.actual==0);
    assert(wait.identity==static_cast<IUnknown*>(&alias.canonical));
    assert(capture.MatchesFence(wait.graph.fence,&alias));
    assert(capture.MatchesFence(wait.graph.fence,&alias.canonical));
    assert(!capture.MatchesFence(wait.graph.fence,&other));
    assert(capture.Signal(queue,&alias,2));
    puts("PASS: separate fence interface/canonical identity matches the exact deferred wait; other fence rejected");
}
struct QueueConsumerTest {
    PTSceneLive::Capture* capture=nullptr;
    PTSceneDescriptors::Acceleration acceleration{};
    unsigned phase=0,resets=0,signals=0;
    ComPtr<ID3D12Fence> privateFence;
    static void Before(void* context,ID3D12CommandQueue*,UINT count,ID3D12CommandList*const*)noexcept{
        auto& self=*static_cast<QueueConsumerTest*>(context);assert(count==1&&self.phase==0);
        PTSceneDispatch::Scene scene;assert(self.capture->Scene(2,self.acceleration,scene)==PTSceneOrder::Status::pending);self.phase=1;
        SetLastError(0x1234); // Observers must not leak their Win32 error state.
    }
    static void Selected(void* context,ID3D12CommandQueue*,UINT count,ID3D12CommandList*const*,bool admitted)noexcept{
        auto& self=*static_cast<QueueConsumerTest*>(context);assert(count==1&&admitted&&self.phase==1);
        PTSceneDispatch::Scene scene;assert(self.capture->Scene(2,self.acceleration,scene)==PTSceneOrder::Status::ready);self.phase=2;
    }
    static void Submitted(void* context,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const*,bool admitted)noexcept{
        auto& self=*static_cast<QueueConsumerTest*>(context);assert(count==1&&admitted&&self.phase==2);
        assert(SUCCEEDED(queue->Signal(self.privateFence.Get(),1)));self.phase=3;
    }
    static void Signalling(void* context,ID3D12CommandQueue*,ID3D12Fence* fence,UINT64 value)noexcept{
        auto& self=*static_cast<QueueConsumerTest*>(context);
        assert(self.phase==3&&value==2&&fence->GetCompletedValue()<value);++self.signals;
        SetLastError(0x9876);
    }
    static void Reset(void* context,ID3D12GraphicsCommandList*)noexcept{
        auto& self=*static_cast<QueueConsumerTest*>(context);++self.resets;
    }
};
static void TestGeometrySnapshot(ID3D12Device* device,bool common=false,bool temporary=false,bool decayedIndex=false){
    const auto sourceState=(common||decayedIndex)?D3D12_RESOURCE_STATE_COMMON:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=1024;desc.Height=1;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const auto make=[&](D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=type;ComPtr<ID3D12Resource> r;
        assert(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r))));return r;
    };
    auto source=make(D3D12_HEAP_TYPE_DEFAULT,sourceState);
    auto upload=make(D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    auto readback=make(D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    unsigned char* mapped=nullptr;D3D12_RANGE empty{};assert(SUCCEEDED(upload->Map(0,&empty,reinterpret_cast<void**>(&mapped))));
    for(unsigned n=0;n<512;++n){mapped[n]=static_cast<unsigned char>(n);mapped[n+512]=0xee;}upload->Unmap(0,nullptr);
    PTOwnedScene::Runtime uploads;const auto uploadAddress=upload->GetGPUVirtualAddress();
    assert(uploads.Created(900,upload.Get(),uploadAddress,desc.Width)==1);
    PTNativeView::Uploads constants;
    auto savedWidth=desc.Width;desc.Width=65536;auto constant=make(D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);desc.Width=savedWidth;
    assert(constants.Created(constant.Get()));assert(constants.Created(constant.Get()));
    void* constantData=nullptr;assert(SUCCEEDED(constant->Map(0,nullptr,&constantData)));
    std::memset(constantData,0x5a,65536);constant->Unmap(0,nullptr);
    unsigned char constantSlice[16]{};const auto constantAddress=constant->GetGPUVirtualAddress();
    assert(constants.Read(constantAddress+3584,constantSlice,sizeof(constantSlice)));
    for(auto byte:constantSlice)assert(byte==0x5a);
    assert(!constants.Read(constantAddress+65530,constantSlice,sizeof(constantSlice)));
    assert(!constants.Created(source.Get()));constant.Reset();
    assert(constants.Read(constantAddress+3584,constantSlice,sizeof(constantSlice))); // Registry owns allocation.
    unsigned char slice[16]{};assert(uploads.ReadUpload(uploadAddress+17,slice,sizeof(slice)));
    for(unsigned n=0;n<16;++n)assert(slice[n]==n+17);
    assert(!uploads.ReadUpload(uploadAddress+desc.Width-8,slice,sizeof(slice)));
    assert(!uploads.ReadUpload(uploadAddress,nullptr,sizeof(slice)));
    auto uploadRelease=uploads.BeforeRelease(900);assert(!uploads.ReadUpload(uploadAddress,slice,sizeof(slice)));assert(uploads.AfterRelease(uploadRelease)==5);
    assert(uploads.Created(901,source.Get(),source->GetGPUVirtualAddress(),desc.Width)==1);
    assert(!uploads.ReadUpload(source->GetGPUVirtualAddress(),slice,sizeof(slice))); // Never map GPU-only memory.
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12CommandQueue> queue;
    const auto commandType=(temporary||decayedIndex)?D3D12_COMMAND_LIST_TYPE_COMPUTE:D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3D12_COMMAND_QUEUE_DESC qd{};qd.Type=commandType;assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    assert(SUCCEEDED(device->CreateCommandAllocator(commandType,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,commandType,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    assert(PTCommandBindings::Watch(list.Get()));PTCommandBindings::tracker.Reset(list.Get(),nullptr);
    const auto transition=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);
    };
    transition(source.Get(),sourceState,D3D12_RESOURCE_STATE_COPY_DEST);
    list->CopyBufferRegion(source.Get(),0,upload.Get(),0,512);
    transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,sourceState);
    if(decayedIndex){
        assert(SUCCEEDED(list->Close()));ID3D12CommandList* init=list.Get();queue->ExecuteCommandLists(1,&init);
        ComPtr<ID3D12Fence> initialized;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&initialized))));
        assert(SUCCEEDED(queue->Signal(initialized.Get(),1)));HANDLE complete=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        assert(complete&&SUCCEEDED(initialized->SetEventOnCompletion(1,complete))&&WaitForSingleObject(complete,30000)==WAIT_OBJECT_0);CloseHandle(complete);
        assert(SUCCEEDED(allocator->Reset()));assert(SUCCEEDED(list->Reset(allocator.Get(),nullptr)));
    }
    std::array<unsigned char,80> wrapper{};const auto address=source->GetGPUVirtualAddress();auto* raw=source.Get();
    const UINT state=decayedIndex?(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_INDEX_BUFFER):(temporary?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:sourceState); // Native default differs from the recorded transition.
    std::memcpy(wrapper.data(),&raw,8);std::memcpy(wrapper.data()+0x20,&state,4);std::memcpy(wrapper.data()+0x30,&desc.Width,8);std::memcpy(wrapper.data()+0x48,&address,8);
    PTOwnedScene::Runtime bank;assert(bank.Created(reinterpret_cast<UINT_PTR>(wrapper.data()),source.Get(),address,desc.Width)==1);
    std::vector<PTSceneTopology::Geometry> geometry{{address,address+256,12,2,3,3,1,1}};
    auto budget=std::make_shared<PTOwnedScene::LeaseBudget>(292);
    std::shared_ptr<PTGeometrySnapshot::Transfer> transfer;
    assert(bank.FreezeGeometry(list.Get(),geometry,budget,transfer)&&budget->Held()==292);
    assert(transfer->storage->copies.size()==1&&geometry[0].frozen); // Same backing copied once.
    std::shared_ptr<PTGeometrySnapshot::Transfer> rejected;
    assert(!bank.FreezeGeometry(list.Get(),geometry,budget,rejected)&&!rejected); // No commands on budget failure.
    // Separate index/vertex allocations: copy the union of needed vertices,
    // rebasing bounded index roots. The old whole-buffer copies
    // cannot fit this deliberately exact range budget.
    PTOwnedScene::Runtime rangedBank;
    auto indexSource=make(D3D12_HEAP_TYPE_DEFAULT,sourceState);
    auto rangeReadback=make(D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    std::array<unsigned char,80> indexWrapper{};auto* indexRaw=indexSource.Get();
    const auto indexAddress=indexSource->GetGPUVirtualAddress();const UINT indexState=sourceState;
    std::memcpy(indexWrapper.data(),&indexRaw,8);std::memcpy(indexWrapper.data()+0x20,&indexState,4);
    std::memcpy(indexWrapper.data()+0x30,&desc.Width,8);std::memcpy(indexWrapper.data()+0x48,&indexAddress,8);
    assert(rangedBank.Created(reinterpret_cast<UINT_PTR>(wrapper.data()),source.Get(),address,desc.Width)==1);
    assert(rangedBank.Created(reinterpret_cast<UINT_PTR>(indexWrapper.data()),indexSource.Get(),indexAddress,desc.Width)==1);
    std::vector<PTSceneTopology::Geometry> rangedGeometry{{indexAddress,address+256,12,2,3,3,2,1},{indexAddress+8,address+320,12,2,3,3,2,1}};
    auto rangeBudget=std::make_shared<PTOwnedScene::LeaseBudget>(116);
    std::shared_ptr<PTGeometrySnapshot::Transfer> rangeTransfer;
    assert(rangedBank.FreezeGeometry(list.Get(),rangedGeometry,rangeBudget,rangeTransfer));
    assert(rangeBudget->Held()==116);
    assert(rangeTransfer->storage->copies.at(2)->GetDesc().Width==16);
    assert(rangedGeometry[0].frozen->indexCopyBase==indexAddress&&rangedGeometry[0].frozen->indexCopyBytes==16);
    assert(rangedGeometry[0].frozen->indexBase==indexAddress&&rangedGeometry[0].frozen->indexBytes==desc.Width);
    assert(rangedGeometry[0].frozen->positionBase==address+256&&rangedGeometry[0].frozen->positionBytes==100);
    assert(rangeTransfer->storage->copies.at(1)->GetDesc().Width==100);
    auto invalidRange=rangedGeometry;invalidRange[0].positions=address+desc.Width-4;
    assert(!rangedBank.FreezeGeometry(list.Get(),invalidRange,rangeBudget,rejected)&&!rejected);
    // An exact six-byte native index resource must never be read as eight bytes.
    desc.Width=6;auto shortIndex=make(D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
    desc.Width=8;auto shortReadback=make(D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);desc.Width=1024;
    list->CopyBufferRegion(shortIndex.Get(),0,upload.Get(),0,6);
    transition(shortIndex.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON);
    PTGeometrySnapshot::Source shortSource{{77,shortIndex->GetGPUVirtualAddress(),6},shortIndex,D3D12_RESOURCE_STATE_COMMON};
    auto shortBudget=std::make_shared<PTOwnedScene::LeaseBudget>(8);
    auto tooSmall=std::make_shared<PTOwnedScene::LeaseBudget>(6);
    std::shared_ptr<PTGeometrySnapshot::Transfer> shortTransfer;
    assert(!PTGeometrySnapshot::Record(list.Get(),{shortSource},tooSmall,shortTransfer));
    assert(PTGeometrySnapshot::Record(list.Get(),{shortSource},shortBudget,shortTransfer));
    auto* shortCopy=shortTransfer->storage->copies.at(77).Get();
    assert(shortCopy->GetDesc().Width==8&&shortBudget->Held()==8&&shortTransfer->sources[0].bytes==6);
    transition(shortCopy,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(shortReadback.Get(),0,shortCopy,0,8);
    transition(shortCopy,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
    transition(source.Get(),(common||decayedIndex)?D3D12_RESOURCE_STATE_COPY_SOURCE:sourceState,D3D12_RESOURCE_STATE_COPY_DEST);
    list->CopyBufferRegion(source.Get(),0,upload.Get(),512,512); // Overwrite the ORIGINAL after its build-time copy.
    transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,sourceState);
    auto* frozen=transfer->storage->copies.at(1).Get();
    transition(frozen,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(readback.Get(),0,frozen,0,292);
    transition(frozen,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
    auto* rangeFrozen=rangeTransfer->storage->copies.at(1).Get();
    transition(rangeFrozen,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(rangeReadback.Get(),0,rangeFrozen,0,100);
    transition(rangeFrozen,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
    assert(SUCCEEDED(list->Close()));
    auto released=bank.BeforeRelease(reinterpret_cast<UINT_PTR>(wrapper.data()));source.Reset();assert(bank.AfterRelease(released)==5);
    PTGeometrySnapshot::Retirement retirement;PTGeometrySnapshot::Retirement::Batch batch;
    assert(retirement.Before(queue.Get(),{transfer,transfer},batch)&&batch.size()==1);
    ComPtr<ID3D12Fence> gate,done;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))));
    assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done))));
    assert(SUCCEEDED(queue->Wait(gate.Get(),1)));ID3D12CommandList* commands=list.Get();queue->ExecuteCommandLists(1,&commands);
    assert(retirement.After(queue.Get(),batch));transfer.reset();geometry.clear();retirement.Poll();assert(budget->Held()==292);
    assert(SUCCEEDED(queue->Signal(done.Get(),1)));assert(SUCCEEDED(gate->Signal(1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event&&SUCCEEDED(done->SetEventOnCompletion(1,event)));
    assert(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
    D3D12_RANGE range{0,292};assert(SUCCEEDED(readback->Map(0,&range,reinterpret_cast<void**>(&mapped))));
    for(unsigned n=0;n<292;++n)assert(mapped[n]==static_cast<unsigned char>(n));readback->Unmap(0,&empty);
    D3D12_RANGE rangedRead{0,100};assert(SUCCEEDED(rangeReadback->Map(0,&rangedRead,reinterpret_cast<void**>(&mapped))));
    for(unsigned n=0;n<100;++n)assert(mapped[n]==static_cast<unsigned char>(n+256));rangeReadback->Unmap(0,&empty);
    D3D12_RANGE shortRange{0,8};assert(SUCCEEDED(shortReadback->Map(0,&shortRange,reinterpret_cast<void**>(&mapped))));
    for(unsigned n=0;n<6;++n)assert(mapped[n]==n);
    unsigned lastWord=0;std::memcpy(&lastWord,mapped+4,4);assert((lastWord&0xffffu)==0x0504u);
    shortReadback->Unmap(0,&empty);shortTransfer.reset();assert(shortBudget->Held()==0);
    rangeTransfer.reset();rangedGeometry.clear();invalidRange.clear();assert(rangeBudget->Held()==0);
    retirement.Poll();assert(budget->Held()==0);
    const auto idle=bank.GeometryStatistics();assert(idle.bytes==292&&idle.allocations==1&&idle.reuses==0);
    assert(SUCCEEDED(allocator->Reset()));assert(SUCCEEDED(list->Reset(allocator.Get(),nullptr)));
    source=make(D3D12_HEAP_TYPE_DEFAULT,sourceState);
    raw=source.Get();const auto nextAddress=source->GetGPUVirtualAddress();
    std::memcpy(wrapper.data(),&raw,8);std::memcpy(wrapper.data()+0x48,&nextAddress,8);
    assert(bank.Created(reinterpret_cast<UINT_PTR>(wrapper.data()),source.Get(),nextAddress,desc.Width)==1);
    transition(source.Get(),sourceState,D3D12_RESOURCE_STATE_COPY_DEST);
    transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,sourceState);
    geometry={{nextAddress,nextAddress+256,12,2,3,3,2,2}};
    assert(bank.FreezeGeometry(list.Get(),geometry,budget,transfer));
    assert(transfer->storage->copies.at(2).Get()==frozen); // Only retired storage re-enters the pool.
    assert(bank.GeometryStatistics().reuses==1&&bank.GeometryStatistics().bytes==0);
    assert(SUCCEEDED(list->Close()));assert(SUCCEEDED(list->Reset(allocator.Get(),nullptr)));
    transfer.reset();geometry.clear();assert(budget->Held()==0);
    assert(bank.GeometryStatistics().bytes==292);
    std::puts("PASS GPU geometry snapshot: build-time bytes survive later source overwrite/release, shared backing copied once, budget failure emits no copy, fence prevents early retirement.");
}
static void TestUntrackedSceneDestinations(ID3D12Device* device){
    PTOwnedScene::Runtime bank;PTSceneLive::Capture capture(bank);
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};
    assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=1024;rd.Height=1;
    rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> resource;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    const auto address=resource->GetGPUVirtualAddress();
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.DestAccelerationStructureData=address;
    // Metadata-only observer calls: no actual AS build or GPU submission.
    capture.Build(list.Get(),&build);capture.Compact(list.Get(),address,address,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE);
    assert(capture.Healthy()&&capture.Statistics().untrackedDestinations==2);
    assert(bank.Created(1,resource.Get(),address,1024)==1);
    PTOwnedScene::Allocation allocation{};assert(bank.Identify(address,256,allocation));
    D3D12_RAYTRACING_GEOMETRY_DESC geometry{};geometry.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometry.Triangles.IndexBuffer=address+512;geometry.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;geometry.Triangles.IndexCount=3;
    geometry.Triangles.VertexBuffer={address+600,12};geometry.Triangles.VertexCount=3;geometry.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;
    build.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;build.Inputs.NumDescs=1;
    build.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;build.Inputs.pGeometryDescs=&geometry;
    capture.Build(list.Get(),&build);
    PTSceneTopology::Instance instance{};instance.idMask=0xff000001;instance.blas=address;instance.transform[0]=1;
    capture.UploadInstances(address+800,&instance,sizeof(instance));
    auto top=build;top.DestAccelerationStructureData=address+256;top.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;top.Inputs.InstanceDescs=address+800;
    capture.Build(list.Get(),&top);assert(capture.Mark(list.Get(),1));ID3D12CommandList* raw=list.Get();
    assert(capture.Submit(queue.Get(),1,&raw));PTSceneDispatch::Scene scene;
    PTSceneDescriptors::Acceleration selected{address+256,allocation.token,768};
    assert(capture.Scene(1,selected,scene)==PTSceneOrder::Status::ready);
    // Ambiguous destination must tombstone every candidate, never expose the
    // preceding geometry and never globally disable unrelated capture.
    capture.Reset(list.Get());assert(bank.Created(2,resource.Get(),address,1024)==1);
    capture.Build(list.Get(),&build);assert(capture.Mark(list.Get(),2));assert(capture.Submit(queue.Get(),1,&raw));
    assert(capture.Healthy()&&capture.Statistics().destinationTombstones==2);
    assert(capture.Scene(2,selected,scene)==PTSceneOrder::Status::missing);
    auto released=bank.BeforeRelease(2);assert(bank.AfterRelease(released)==5);
    capture.Reset(list.Get());capture.Build(list.Get(),&build);assert(capture.Mark(list.Get(),3));assert(capture.Submit(queue.Get(),1,&raw));
    assert(capture.Scene(3,selected,scene)==PTSceneOrder::Status::ready);
    // A tracked but undersized destination is also tombstoned, not ignored.
    capture.Reset(list.Get());auto shortBuild=build;shortBuild.DestAccelerationStructureData=address+1000;
    capture.Build(list.Get(),&shortBuild);assert(capture.Healthy()&&capture.Statistics().destinationTombstones==3);
    std::puts("PASS: untracked AS writes stay local; aliases and short destinations tombstone; later valid builds recover");
}

static void TestOffUpdateCapture(ID3D12Device* device){
    PTOwnedScene::Runtime bank;PTSceneLive::Capture capture(bank);
    assert(capture.PreserveGeometry());capture.RenderingRequested(false);
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=1024;desc.Height=1;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const auto make=[&](D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=type;ComPtr<ID3D12Resource> out;
        assert(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&out))));return out;
    };
    auto source=make(D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON);
    auto upload=make(D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    unsigned char* bytes=nullptr;assert(SUCCEEDED(upload->Map(0,nullptr,reinterpret_cast<void**>(&bytes))));
    for(unsigned n=0;n<1024;++n)bytes[n]=static_cast<unsigned char>(n);upload->Unmap(0,nullptr);
    const auto address=source->GetGPUVirtualAddress();auto* resource=source.Get();std::array<unsigned char,80> wrapper{};
    std::memcpy(wrapper.data(),&resource,8);std::memcpy(wrapper.data()+0x30,&desc.Width,8);std::memcpy(wrapper.data()+0x48,&address,8);
    assert(bank.Created(reinterpret_cast<UINT_PTR>(wrapper.data()),source.Get(),address,desc.Width)==1);
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    list->CopyBufferRegion(source.Get(),0,upload.Get(),0,1024);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={source.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON};list->ResourceBarrier(1,&barrier);
    assert(PTCommandBindings::Watch(list.Get()));PTCommandBindings::tracker.Reset(list.Get(),nullptr);
    D3D12_RAYTRACING_GEOMETRY_DESC geometry{};geometry.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometry.Triangles.IndexBuffer=address;geometry.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;geometry.Triangles.IndexCount=3;
    geometry.Triangles.VertexBuffer={address+256,12};geometry.Triangles.VertexCount=3;geometry.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.DestAccelerationStructureData=address+512;
    build.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;build.Inputs.NumDescs=1;
    build.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;build.Inputs.pGeometryDescs=&geometry;
    capture.Build(list.Get(),&build);
    geometry.Triangles.VertexBuffer.StartAddress=address+320;build.SourceAccelerationStructureData=build.DestAccelerationStructureData;
    build.Inputs.Flags=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    capture.Build(list.Get(),&build); // Last update happens while PT is off.
    PTSceneTopology::Instance instance{};instance.idMask=0x01000001;instance.blas=address+512;instance.transform[0]=1;
    capture.UploadInstances(address+128,&instance,sizeof(instance));
    auto top=build;top.DestAccelerationStructureData=address+768;top.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;top.Inputs.InstanceDescs=address+128;
    capture.Build(list.Get(),&top);assert(capture.Mark(list.Get(),1));
    capture.RenderingRequested(true); // No subsequent geometry update/rebuild.
    ComPtr<ID3D12Fence> cpuFence;assert(SUCCEEDED(device->CreateFence(5,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&cpuFence))));
    assert(SUCCEEDED(queue->Wait(cpuFence.Get(),5))&&capture.Wait(queue.Get(),cpuFence.Get(),5));
    assert(capture.Statistics().completedWaits==1); // Initial value, no queue Signal.
    assert(SUCCEEDED(queue->Wait(cpuFence.Get(),6))&&capture.Wait(queue.Get(),cpuFence.Get(),6));
    assert(capture.Statistics().completedWaits==1);
    assert(SUCCEEDED(cpuFence->Signal(6))); // CPU signal is discovered non-blockingly by Submit.
    ID3D12CommandList* raw=list.Get();assert(capture.Submit(queue.Get(),1,&raw));
    PTOwnedScene::Allocation allocation;assert(bank.Identify(address+768,256,allocation));PTSceneDispatch::Scene scene;
    assert(capture.Scene(1,{address+768,allocation.token,256},scene)==PTSceneOrder::Status::ready);
    assert(capture.Statistics().completedWaits==2);
    PTNativeBinding::Binding binding{};binding.indices=address;binding.constants[1]=2;binding.constants[8]=6;
    std::vector<PTSceneTopology::Mapping> mapping;scene.topology.Check(scene.instances,{binding},2,&mapping);
    assert(mapping.size()==1&&mapping[0].referenced&&mapping[0].geometry.positions==address+320);
    const auto frozen=mapping[0].geometry.frozen;assert(frozen&&capture.Statistics().offUpdatesPreserved==1&&capture.Statistics().copyFailures==0);
    assert(SUCCEEDED(list->Close()));queue->ExecuteCommandLists(1,&raw);capture.Submitted(queue.Get());
    ComPtr<ID3D12Fence> done;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done))));
    assert(SUCCEEDED(queue->Signal(done.Get(),1)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    assert(event&&SUCCEEDED(done->SetEventOnCompletion(1,event))&&WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
    capture.Reset(list.Get());assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    std::puts("PASS: last BLAS update while PT off stays frozen and selectable after enabling without another build");
}
static void TestUnrelatedSceneLists(ID3D12Device* device){
    PTOwnedScene::Runtime bank;PTSceneLive::Capture capture(bank);
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC desc{};
    assert(SUCCEEDED(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue))));
    for(unsigned n=0;n<320;++n){
        ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
        assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
        assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
        capture.Reset(list.Get());assert(SUCCEEDED(list->Close()));ID3D12CommandList* raw=list.Get();
        // Metadata-only submission: these empty lists have no GPU references.
        assert(capture.Submit(queue.Get(),1,&raw));assert(capture.Healthy());
    }
    assert(capture.Statistics().healthy);
    std::puts("PASS: 320 unrelated native lists do not exhaust the scene journal");
}

// Model a second COM interface address with the SAME controlling IUnknown.
// Tracker only calls IUnknown on it; no fake command ever reaches the device.
struct BindingAlias final : IUnknown {
    IUnknown* owner;unsigned queries=0;
    explicit BindingAlias(IUnknown* value):owner(value){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {++queries;return owner->QueryInterface(iid,out);}
    ULONG STDMETHODCALLTYPE AddRef() override {return 2;}
    ULONG STDMETHODCALLTYPE Release() override {return 1;}
};
static void TestBindingFirstObservation(ID3D12Device* device){
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=256;desc.Height=1;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> buffer;assert(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&buffer))));
    PTCommandBindings::Tracker tracker;
    // No explicit Watch: the first native setter must establish the entry.
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={buffer.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST};
    tracker.Barriers(list.Get(),1,&barrier);
    D3D12_RESOURCE_STATES state{};
    assert(tracker.BufferState(list.Get(),buffer.Get(),D3D12_RESOURCE_STATE_COMMON,state)&&state==D3D12_RESOURCE_STATE_COPY_DEST);
    BindingAlias alias(list.Get());auto* other=reinterpret_cast<ID3D12GraphicsCommandList*>(&alias);
    assert(tracker.BufferState(other,buffer.Get(),D3D12_RESOURCE_STATE_COMMON,state)&&state==D3D12_RESOURCE_STATE_COPY_DEST);
    assert(alias.queries==1);
    barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;tracker.Barriers(other,1,&barrier);
    assert(tracker.BufferState(list.Get(),buffer.Get(),D3D12_RESOURCE_STATE_COMMON,state)&&state==D3D12_RESOURCE_STATE_COPY_SOURCE);
    assert(alias.queries==1); // Registered interfaces stay on the no-QI hot path.
    tracker.Reset(other,nullptr);
    assert(tracker.BufferState(list.Get(),buffer.Get(),D3D12_RESOURCE_STATE_COMMON,state)&&state==D3D12_RESOURCE_STATE_COMMON);
    assert(SUCCEEDED(list->Close()));
    std::puts("PASS binding first observation: unregistered setters retained, alternate COM interface shares state and Reset, no repeated QueryInterface.");
}
int main(){
    ComPtr<ID3D12Debug> debug;const bool debugEnabled=SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
    if(debugEnabled)debug->EnableDebugLayer();
    ComPtr<IDXGIFactory4> f;assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&f))));
    ComPtr<IDXGIAdapter> a;assert(SUCCEEDED(f->EnumWarpAdapter(IID_PPV_ARGS(&a))));
    ComPtr<ID3D12Device> d;assert(SUCCEEDED(D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d))));
    TestBindingFirstObservation(d.Get());
    TestUnrelatedSceneLists(d.Get());
    TestUntrackedSceneDestinations(d.Get());
    TestGeometrySnapshot(d.Get());TestGeometrySnapshot(d.Get(),true);TestGeometrySnapshot(d.Get(),false,true);TestGeometrySnapshot(d.Get(),false,false,true);
    TestOffUpdateCapture(d.Get());
    if(debugEnabled){
        ComPtr<ID3D12InfoQueue> messages;assert(SUCCEEDED(d.As(&messages)));
        for(UINT64 n=0;n<messages->GetNumStoredMessages();++n){SIZE_T bytes=0;messages->GetMessage(n,nullptr,&bytes);
            std::vector<unsigned char> storage(bytes);auto* message=reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            assert(SUCCEEDED(messages->GetMessage(n,message,&bytes)));
            if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR){std::puts(message->pDescription);assert(false);}
        }
        std::puts("PASS: geometry snapshots produced no D3D12 debug-layer errors.");
    }else std::puts("D3D12 debug layer unavailable; byte preservation and fence tests still executed.");
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=65536;rd.Height=1;
    rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> resource;
    assert(SUCCEEDED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    PTResourceLeaseProbe::Probe probe;
    auto invalid=probe.Before(resource.Get(),resource->GetGPUVirtualAddress()+1,65536);
    assert(probe.After(invalid).status==3&&probe.HeldBytes()==0);
    auto lease=probe.Before(resource.Get(),resource->GetGPUVirtualAddress(),65536);
    assert(lease.resource&&probe.HeldBytes()==65536);
    resource.Reset(); // Drop the original owner's final reference.
    assert(probe.After(lease).status==5&&probe.HeldBytes()==0);
    assert(probe.After(lease).status==0); // Cannot double-release a completed lease.
    auto missing=probe.Before(nullptr,0,0);assert(probe.After(missing).status==0);
    PTResourceLeaseProbe::Probe budget;
    rd.Width=PTResourceLeaseProbe::Probe::MaximumResource;
    assert(SUCCEEDED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    PTResourceLeaseProbe::Lease held[9]{};
    for(auto& l:held)l=budget.Before(resource.Get(),resource->GetGPUVirtualAddress(),rd.Width);
    assert(budget.HeldBytes()==PTResourceLeaseProbe::Probe::MaximumHeld&&held[8].status==4);
    resource.Reset();for(auto& l:held)budget.After(l);assert(budget.HeldBytes()==0);
    PTOwnedScene::Runtime scene;
    assert(SUCCEEDED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    const auto address=resource->GetGPUVirtualAddress();
    assert(scene.Created(123,resource.Get(),address,rd.Width)==1);
    auto retained=scene.BeforeRelease(123);
    resource.Reset(); // Metadata can now be cleared; retained identity is independent.
    assert(scene.AfterRelease(retained)==5);
    assert(scene.AfterRelease(retained)==0);
    const auto stats=scene.Stop();assert(stats.retained==1&&stats.validatedRelease==1&&stats.mismatchedRelease==0);
    assert(scene.Created(124,nullptr,0,0)==0); // Closed capture cannot acquire references.
    // One real WARP buffer backs all selected fixture ranges. This is a
    // lifetime test, not an AS build or rendering validation.
    rd.Width=65536;
    assert(SUCCEEDED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    const auto base=resource->GetGPUVirtualAddress();
    auto ledger=std::make_shared<PTOwnedScene::LeaseBudget>(2*rd.Width);
    PTOwnedScene::Runtime bank(ledger);assert(bank.Created(200,resource.Get(),base,rd.Width)==1);
    PTSceneObserver::Event event{};event.index=base;event.position=base+256;event.stride=12;
    event.indexFormat=57;event.vertexFormat=6;event.indexCount=event.vertexCount=3;bank.Geometry(event);
    PTSceneDispatch::Scene frozen;frozen.address=base+1024;frozen.version=1;frozen.tlas={1,256};frozen.blas[base+2048]={1,256};
    assert(frozen.topology.Build(base+2048,{{base,base+256,12,2,3,3,1,1}}));
    PTSceneTopology::Instance selectedInstance{};selectedInstance.idMask=0xff000001;selectedInstance.blas=base+2048;frozen.instances={selectedInstance};
    PTSceneDispatch::Table table;table.address=0xc000;table.version=1;table.pipeline=3;table.rays=2;
    PTNativeBinding::Binding row{};row.indices=base;row.attributes=base+512;
    row.constants[1]=2;row.constants[2]=16;row.constants[6]=8;row.constants[8]=6;row.constants[9]=64;
    table.bindings={row};table.allocationTokens={{{1,1}}};table.bytes.resize(192);
    PTSceneDispatch::ExportSet exports{3,4,std::vector<PTShaderTableAdapter::Exports>(2)};
    for(unsigned ray=0;ray<2;++ray){auto& e=exports.rays[ray];e.nativeOpaque[0]=1;e.nativeAlpha[0]=2;e.privateOpaque[0]=3;e.privateAlpha[0]=4;
        auto* out=table.bytes.data()+ray*96;std::memcpy(out,e.nativeOpaque.data(),32);std::memcpy(out+32,&row.indices,56);}
    // Reconstruct actual native writer bytes while its CPU pointers are live.
    std::array<unsigned char,80> writerBuffer{};auto* writerResource=resource.Get();
    std::memcpy(writerBuffer.data(),&writerResource,8);std::memcpy(writerBuffer.data()+0x30,&rd.Width,8);std::memcpy(writerBuffer.data()+0x48,&base,8);
    std::array<unsigned char,64> writerState{};const UINT64 writerPipeline=3;std::memcpy(writerState.data(),&writerPipeline,8);
    std::array<PTShaderTableAdapter::Identifier,4> writerIds{exports.rays[0].nativeOpaque,exports.rays[1].nativeOpaque,exports.rays[0].nativeAlpha,exports.rays[1].nativeAlpha};
    for(unsigned i=0;i<4;++i){const void* id=writerIds[i].data();std::memcpy(writerState.data()+0x20+i*8,&id,8);}
    PTNativeJobRoute::Ticket writerTicket;writerTicket.serial=17;writerTicket.pipeline=writerPipeline;
    writerTicket.writer={reinterpret_cast<UINT_PTR>(writerBuffer.data()),reinterpret_cast<UINT_PTR>(writerState.data()),reinterpret_cast<UINT_PTR>(&row),320,2,1};
    writerTicket.layout={base,rd.Width,32,64,32,64,96,2};writerTicket.hitTable=base+128;writerTicket.hitBytes=192;
    PTNativeShaderTable::Identifiers writerIdentifiers;PTSceneDispatch::Table writerTable;
    assert(bank.SnapshotWriter(writerTicket,writerIdentifiers,writerTable));
    assert(writerTable.bytes==table.bytes&&writerTable.allocationTokens==table.allocationTokens&&writerIdentifiers.hit==writerIds);
    writerIds[0][0]=88;assert(writerIdentifiers.hit[0][0]==1); // Immutable ID snapshot.
    const char* writerFailure="none";std::size_t writerRow=999;
    auto missingRow=row;missingRow.attributes=0;
    writerTicket.writer.bindings=reinterpret_cast<UINT_PTR>(&missingRow);
    assert(bank.SnapshotWriter(writerTicket,writerIdentifiers,writerTable,&writerFailure,&writerRow));
    assert(!std::strcmp(writerFailure,"none")&&writerTable.allocationTokens[0][1]==0);
    PTSceneDispatch::Plan missingPlan;
    const PTSceneDispatch::Selection missingSelection{frozen.address,frozen.version,writerTable.address,writerTable.version,writerTable.pipeline};
    PTOwnedScene::LeaseBatch missingLease;
    assert(!bank.PrepareLease(missingSelection,frozen,writerTable,exports,missingPlan,missingLease)&&!missingLease.Count());
    writerTicket.writer.bindings=reinterpret_cast<UINT_PTR>(&row);
    writerTicket.hitBytes=191;assert(!bank.SnapshotWriter(writerTicket,writerIdentifiers,writerTable)&&writerTable.bytes.empty());
    std::puts("PASS writer snapshot: exact recorded layout, native identifiers, local-root bytes and allocation generations frozen inside owner transaction.");
    PTSceneDispatch::Selection selection{frozen.address,1,table.address,1,3};PTSceneDispatch::Plan plan;
    PTOwnedScene::LeaseBatch first,second,third;
    assert(bank.PrepareLease(selection,frozen,table,exports,plan,first)&&first.Count()==1&&first.Bytes()==rd.Width);
    assert(bank.PrepareLease(selection,frozen,table,exports,plan,second));
    assert(!bank.PrepareLease(selection,frozen,table,exports,plan,third)&&!third.Count()&&plan.records.empty());
    assert(ledger->Held()==2*rd.Width);
    D3D12_COMMAND_QUEUE_DESC queueDesc{};ComPtr<ID3D12CommandQueue> queue;
    assert(SUCCEEDED(d->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12Fence> gate,done;assert(SUCCEEDED(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))));
    assert(SUCCEEDED(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done))));
    PTOwnedScene::Submissions submissions;assert(submissions.Initialize(queue.Get()));
    auto additional=std::make_shared<int>(12);std::weak_ptr<int> additionalLifetime=additional;
    assert(first.Attach(additional));additional.reset();
    assert(submissions.Hold(100,first)&&!first.Count());assert(!additionalLifetime.expired());
    assert(submissions.Hold(101,second));submissions.Discarded(101);assert(ledger->Held()==rd.Width);
    assert(SUCCEEDED(queue->Wait(gate.Get(),1)));
    // No resource work is needed to prove retirement cannot pass a queued wait.
    assert(submissions.Submitted(100));assert(!submissions.Submitted(100));
    submissions.Discarded(100);submissions.Poll();assert(ledger->Held()==rd.Width);
    auto* retainedResource=resource.Get();
    auto release=bank.BeforeRelease(200);resource.Reset();assert(bank.AfterRelease(release)==5);
    bank.Stop();assert(ledger->Held()==rd.Width); // Registry shutdown cannot retire GPU leases.
    assert(retainedResource->GetDesc().Width==rd.Width&&retainedResource->GetGPUVirtualAddress()==base);
    assert(!bank.PrepareLease(selection,frozen,table,exports,plan,third));
    assert(SUCCEEDED(queue->Signal(done.Get(),1)));assert(SUCCEEDED(gate->Signal(1)));
    HANDLE ready=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(ready);
    assert(SUCCEEDED(done->SetEventOnCompletion(1,ready)));assert(WaitForSingleObject(ready,30000)==WAIT_OBJECT_0);CloseHandle(ready);
    submissions.Poll();assert(ledger->Held()==0&&additionalLifetime.expired());
    std::puts("PASS dispatch leases: exact atomic retention, deduplicated bytes, bounded batches, unsubmitted discard, owner shutdown and actual queue-fence retirement.");
    PTSceneLive::Owner live;assert(live.Start(d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),false));
    assert(SUCCEEDED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};heapDesc.NumDescriptors=64;heapDesc.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap;assert(SUCCEEDED(d->CreateDescriptorHeap(&heapDesc,IID_PPV_ARGS(&heap))));
    const UINT step=d->GetDescriptorHandleIncrementSize(heapDesc.Type);
    const auto handle=heap->GetCPUDescriptorHandleForHeapStart();
    const UINT64 asHandle=handle.ptr+10ull*step,liveAddress=resource->GetGPUVirtualAddress();
    std::array<unsigned char,80> wrapper{};auto* pointer=resource.Get();
    std::memcpy(wrapper.data(),&pointer,8);std::memcpy(wrapper.data()+0x10,&asHandle,8);
    std::memcpy(wrapper.data()+0x30,&rd.Width,8);std::memcpy(wrapper.data()+0x48,&liveAddress,8);
    live.Created(wrapper.data(),true);PTSceneDescriptors::Acceleration selectedAS;
    assert(live.Select(heap.Get(),0,5,selectedAS)&&selectedAS.address==liveAddress);
    const auto originalToken=selectedAS.token;
    PTSceneLive::Owner::SelectionDiagnostic selectionDetail;
    assert(!live.Select(heap.Get(),20,5,selectedAS,&selectionDetail));
    assert(std::strcmp(selectionDetail.reason,"descriptor_missing")==0&&selectionDetail.handle==handle.ptr+30ull*step);
    assert(live.Select(heap.Get(),0,5,selectedAS,&selectionDetail));
    assert(selectionDetail.token==originalToken&&selectionDetail.currentToken==originalToken&&std::strcmp(selectionDetail.reason,"none")==0);
    // Native capture adapter uses canonical COM list identity, snapshots CPU
    // instance rows, and joins them only when the observed list is submitted.
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    assert(SUCCEEDED(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    auto& capture=live.Scenes();capture.Reset(list.Get());
    D3D12_RAYTRACING_GEOMETRY_DESC nativeGeometry{};nativeGeometry.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    nativeGeometry.Triangles.IndexBuffer=liveAddress+256;nativeGeometry.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;nativeGeometry.Triangles.IndexCount=3;
    nativeGeometry.Triangles.VertexBuffer={liveAddress+512,12};nativeGeometry.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;nativeGeometry.Triangles.VertexCount=3;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC nativeBuild{};nativeBuild.DestAccelerationStructureData=liveAddress+2048;
    nativeBuild.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;nativeBuild.Inputs.NumDescs=1;nativeBuild.Inputs.pGeometryDescs=&nativeGeometry;
    capture.Build(list.Get(),&nativeBuild);
    capture.Compact(list.Get(),liveAddress+4096,liveAddress+2048,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
    PTSceneTopology::Instance nativeInstance{};nativeInstance.idMask=0xff000001;nativeInstance.blas=liveAddress+4096;nativeInstance.transform[0]=1;
    capture.UploadInstances(liveAddress+8192,&nativeInstance,sizeof(nativeInstance));
    nativeBuild={};nativeBuild.DestAccelerationStructureData=liveAddress;
    nativeBuild.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;nativeBuild.Inputs.NumDescs=1;nativeBuild.Inputs.InstanceDescs=liveAddress+8192;
    capture.Build(list.Get(),&nativeBuild);assert(capture.Mark(list.Get(),1));
    nativeInstance.transform[0]=99; // Copying source rows must not change the build snapshot.
    PTSceneDispatch::Scene selectedScene;assert(capture.Scene(1,selectedAS,selectedScene)==PTSceneOrder::Status::pending);
    ID3D12CommandList* submittedList=list.Get();assert(capture.Submit(queue.Get(),1,&submittedList));
    assert(capture.Scene(1,selectedAS,selectedScene)==PTSceneOrder::Status::ready);
    assert(selectedScene.instances.size()==1&&selectedScene.instances[0].transform[0]==1);
    assert(selectedScene.blas.at(liveAddress+4096).token==originalToken);
    PTNativeBinding::Binding selectedBinding{};selectedBinding.indices=liveAddress+256;selectedBinding.constants[1]=2;selectedBinding.constants[8]=6;
    std::vector<PTSceneTopology::Mapping> selectedMapping;
    selectedScene.topology.Check(selectedScene.instances,{selectedBinding},2,&selectedMapping);
    assert(selectedMapping.size()==1&&selectedMapping[0].geometry.positions==liveAddress+512&&selectedMapping[0].geometry.positionToken==originalToken);
    // This checks the observer, not AS execution: no synthetic build is sent to WARP.
    assert(SUCCEEDED(list->Close()));
    std::puts("PASS native scene capture: immutable instance upload, geometry identities, compaction source and submitted list selection.");
    assert(PTSceneQueues::Start(&capture)&&PTSceneQueues::WatchQueue(queue.Get())&&PTSceneQueues::WatchList(list.Get()));
    QueueConsumerTest client;client.capture=&capture;client.acceleration=selectedAS;
    assert(SUCCEEDED(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&client.privateFence))));
    const PTSceneQueues::Consumer callbacks{&client,&QueueConsumerTest::Before,&QueueConsumerTest::Selected,&QueueConsumerTest::Submitted,&QueueConsumerTest::Reset,&QueueConsumerTest::Signalling};
    assert(PTSceneQueues::Register(&capture,&callbacks));assert(!PTSceneQueues::Register(&capture,&callbacks));
    assert(SUCCEEDED(list->Reset(allocator.Get(),nullptr))); // Real Reset invokes the generation observer.
    assert(client.resets==1);
    capture.Build(list.Get(),&nativeBuild);assert(capture.Mark(list.Get(),2));
    assert(SUCCEEDED(list->Close()));queue->ExecuteCommandLists(1,&submittedList);
    assert(client.phase==3);
    assert(SUCCEEDED(queue->Signal(done.Get(),2)));assert(SUCCEEDED(queue->Wait(done.Get(),2)));
    assert(capture.Scene(2,selectedAS,selectedScene)==PTSceneOrder::Status::ready);
    ready=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(ready);
    assert(SUCCEEDED(done->SetEventOnCompletion(2,ready)));assert(WaitForSingleObject(ready,30000)==WAIT_OBJECT_0);CloseHandle(ready);
    assert(client.privateFence->GetCompletedValue()==1&&client.signals==1);
    TestFenceIdentity(live.Resources(),queue.Get(),list.Get());
    // Stop observing before this stack-owned fixture ends. Production capture
    // has process lifetime; installed forwarding hooks remain valid either way.
    PTSceneQueues::consumer.store(nullptr);PTSceneQueues::observer.store(nullptr);
    std::puts("PASS renderer queue callbacks: descriptor selection before catalog submission, scene ready before Execute, retirement after Execute, Reset notification, single registration and nested private fence forwarding.");
    std::puts("PASS scene queue interception: real WARP Reset, ExecuteCommandLists, Signal and Wait preserve forwarding and select the submitted scene.");
    live.Copy({{handle.ptr+11ull*step,1}},{{asHandle,1}});assert(live.Select(heap.Get(),1,5,selectedAS));
    auto oldOwner=live.BeforeRelease(wrapper.data());live.AfterRelease(oldOwner);
    assert(!live.Select(heap.Get(),1,5,selectedAS));
    live.Created(wrapper.data(),true);assert(live.Select(heap.Get(),0,5,selectedAS)&&selectedAS.token!=originalToken);
    assert(!live.Select(heap.Get(),1,5,selectedAS)); // Identical VA cannot revive copied old identity.
    live.Overwrite({static_cast<SIZE_T>(asHandle)});assert(!live.Select(heap.Get(),0,5,selectedAS));
    live.Created(wrapper.data(),true);live.Heap(heap.Get());assert(!live.Select(heap.Get(),0,5,selectedAS));
    std::puts("PASS session owner: native AS descriptor seeding, exact slot selection, release/reuse rejection and heap invalidation.");
    PTSceneTopology::Runtime topology;
    PTSceneTopology::Instance instance{};instance.idMask=0xff000001;instance.blas=0xa000;
    topology.UploadInstances(0x5000,&instance,sizeof(instance));
    D3D12_RAYTRACING_GEOMETRY_DESC triangle{};triangle.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    triangle.Triangles.IndexBuffer=0x1000;triangle.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;triangle.Triangles.IndexCount=3;
    triangle.Triangles.VertexBuffer={0x2000,12};triangle.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;triangle.Triangles.VertexCount=3;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.DestAccelerationStructureData=0xa000;
    build.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;build.Inputs.NumDescs=1;build.Inputs.pGeometryDescs=&triangle;
    topology.Build(&build);build={};build.DestAccelerationStructureData=0xb000;
    build.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;build.Inputs.NumDescs=1;build.Inputs.InstanceDescs=0x5000;
    topology.Build(&build);PTNativeBinding::Binding binding{};binding.indices=0x1000;binding.constants[1]=2;binding.constants[8]=8;
    topology.Table({binding},2);topology.UploadInstances(0x6000,reinterpret_cast<void*>(1),64);
    const auto topologyStats=topology.Stop();assert(topologyStats.relation.matched==1&&topologyStats.uploads==1&&topologyStats.readErrors==1);
    std::puts("PASS M11 WARP and topology: owned reference survives native-owner release; address mismatch, byte budget and duplicate cleanup rejected.");
}
