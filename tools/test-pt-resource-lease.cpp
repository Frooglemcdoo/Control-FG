#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_resource_lease_probe.h"
#include "../src/pt_owned_scene_runtime.h"
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
