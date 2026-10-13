#include "../src/pt_native_uploads.h"
#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
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
struct QueueConsumerTest {
    PTSceneLive::Capture* capture=nullptr;
    PTSceneDescriptors::Acceleration acceleration{};
    unsigned phase=0,resets=0;
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
    static void Reset(void* context,ID3D12GraphicsCommandList*)noexcept{
        auto& self=*static_cast<QueueConsumerTest*>(context);++self.resets;
    }
};
static void TestGeometrySnapshot(ID3D12Device* device){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=1024;desc.Height=1;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const auto make=[&](D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=type;ComPtr<ID3D12Resource> r;
        assert(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r))));return r;
    };
    auto source=make(D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
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
    D3D12_COMMAND_QUEUE_DESC qd{};assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    const auto transition=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);
    };
    transition(source.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
    list->CopyBufferRegion(source.Get(),0,upload.Get(),0,512);
    transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    std::array<unsigned char,80> wrapper{};const auto address=source->GetGPUVirtualAddress();auto* raw=source.Get();
    const UINT state=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    std::memcpy(wrapper.data(),&raw,8);std::memcpy(wrapper.data()+0x20,&state,4);std::memcpy(wrapper.data()+0x30,&desc.Width,8);std::memcpy(wrapper.data()+0x48,&address,8);
    PTOwnedScene::Runtime bank;assert(bank.Created(reinterpret_cast<UINT_PTR>(wrapper.data()),source.Get(),address,desc.Width)==1);
    std::vector<PTSceneTopology::Geometry> geometry{{address,address+256,12,2,3,3,1,1}};
    auto budget=std::make_shared<PTOwnedScene::LeaseBudget>(desc.Width);
    std::shared_ptr<PTGeometrySnapshot::Transfer> transfer;
    assert(bank.FreezeGeometry(list.Get(),geometry,budget,transfer)&&budget->Held()==desc.Width);
    assert(transfer->storage->copies.size()==1&&geometry[0].frozen); // Same backing copied once.
    std::shared_ptr<PTGeometrySnapshot::Transfer> rejected;
    assert(!bank.FreezeGeometry(list.Get(),geometry,budget,rejected)&&!rejected); // No commands on budget failure.
    transition(source.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
    list->CopyBufferRegion(source.Get(),0,upload.Get(),512,512); // Overwrite the ORIGINAL after its build-time copy.
    transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    auto* frozen=transfer->storage->copies.at(1).Get();
    transition(frozen,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(readback.Get(),0,frozen,0,512);
    transition(frozen,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
    assert(SUCCEEDED(list->Close()));
    auto released=bank.BeforeRelease(reinterpret_cast<UINT_PTR>(wrapper.data()));source.Reset();assert(bank.AfterRelease(released)==5);
    PTGeometrySnapshot::Retirement retirement;PTGeometrySnapshot::Retirement::Batch batch;
    assert(retirement.Before(queue.Get(),{transfer},batch));
    ComPtr<ID3D12Fence> gate,done;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))));
    assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done))));
    assert(SUCCEEDED(queue->Wait(gate.Get(),1)));ID3D12CommandList* commands=list.Get();queue->ExecuteCommandLists(1,&commands);
    assert(retirement.After(queue.Get(),batch));transfer.reset();geometry.clear();retirement.Poll();assert(budget->Held()==desc.Width);
    assert(SUCCEEDED(queue->Signal(done.Get(),1)));assert(SUCCEEDED(gate->Signal(1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event&&SUCCEEDED(done->SetEventOnCompletion(1,event)));
    assert(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
    D3D12_RANGE range{0,512};assert(SUCCEEDED(readback->Map(0,&range,reinterpret_cast<void**>(&mapped))));
    for(unsigned n=0;n<512;++n)assert(mapped[n]==static_cast<unsigned char>(n));readback->Unmap(0,&empty);
    retirement.Poll();assert(budget->Held()==0);
    std::puts("PASS GPU geometry snapshot: build-time bytes survive later source overwrite/release, shared backing copied once, budget failure emits no copy, fence prevents early retirement.");
}
int main(){
    ComPtr<IDXGIFactory4> f;assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&f))));
    ComPtr<IDXGIAdapter> a;assert(SUCCEEDED(f->EnumWarpAdapter(IID_PPV_ARGS(&a))));
    ComPtr<ID3D12Device> d;assert(SUCCEEDED(D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d))));
    TestGeometrySnapshot(d.Get());
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
    const PTSceneQueues::Consumer callbacks{&client,&QueueConsumerTest::Before,&QueueConsumerTest::Selected,&QueueConsumerTest::Submitted,&QueueConsumerTest::Reset};
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
    assert(client.privateFence->GetCompletedValue()==1);
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
