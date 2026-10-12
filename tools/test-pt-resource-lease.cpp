#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_resource_lease_probe.h"
#include "../src/pt_owned_scene_runtime.h"
#include "../src/pt_scene_submissions.h"
#include "../src/pt_scene_topology_runtime.h"
using Microsoft::WRL::ComPtr;
int main(){
    ComPtr<IDXGIFactory4> f;assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&f))));
    ComPtr<IDXGIAdapter> a;assert(SUCCEEDED(f->EnumWarpAdapter(IID_PPV_ARGS(&a))));
    ComPtr<ID3D12Device> d;assert(SUCCEEDED(D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d))));
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
    assert(submissions.Hold(100,first)&&!first.Count());
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
    submissions.Poll();assert(ledger->Held()==0);
    std::puts("PASS dispatch leases: exact atomic retention, deduplicated bytes, bounded batches, unsubmitted discard, owner shutdown and actual queue-fence retirement.");
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
