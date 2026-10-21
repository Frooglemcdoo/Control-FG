#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "../src/rt_lights/layout.h"
using Microsoft::WRL::ComPtr;
using namespace rt_lights;
static void Check(HRESULT value) {
    if(FAILED(value)) throw std::runtime_error("HRESULT " + std::to_string(static_cast<unsigned long>(value)));
}
static void Require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
static std::vector<char> Read(const char* path) {
    std::ifstream file(path,std::ios::binary);
    Require(bool(file),path);
    return std::vector<char>((std::istreambuf_iterator<char>(file)),{});
}
struct Device {
    ComPtr<ID3D12Device5> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList4> list;
    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12DescriptorHeap> views;
    UINT descriptorSize=0;
    std::vector<ComPtr<ID3D12Resource>> retained;
    Device() {
        ComPtr<IDXGIFactory6> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        for(UINT i=0;;++i) {
            ComPtr<IDXGIAdapter1> adapter;
            if(factory->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter))==DXGI_ERROR_NOT_FOUND) break;
            DXGI_ADAPTER_DESC1 desc{}; Check(adapter->GetDesc1(&desc));
            if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            ComPtr<ID3D12Device5> candidate;
            if(FAILED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&candidate)))) continue;
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};
            if(FAILED(candidate->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&options,sizeof(options))) || options.RaytracingTier==D3D12_RAYTRACING_TIER_NOT_SUPPORTED) continue;
            device=candidate; std::wcout<<L"GPU: "<<desc.Description<<L'\n'; break;
        }
        Require(bool(device),"No hardware DXR adapter. This test requires a ray-tracing GPU.");
        D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
        Check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
        Check(device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&allocator)));
        Check(device->CreateCommandList(0,q.Type,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
        Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
        D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heap.NumDescriptors=16;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Check(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&views)));
        descriptorSize=device->GetDescriptorHandleIncrementSize(heap.Type);
        ID3D12DescriptorHeap* heaps[]={views.Get()};list->SetDescriptorHeaps(1,heaps);
    }
    ComPtr<ID3D12Resource> Buffer(UINT64 bytes,D3D12_HEAP_TYPE heapType,D3D12_RESOURCE_STATES state,bool uav=false,const void* data=nullptr) {
        D3D12_HEAP_PROPERTIES heap{};heap.Type=heapType;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;
        desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(uav)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ComPtr<ID3D12Resource> resource;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&resource)));
        if(data) { void* mapped=nullptr;D3D12_RANGE empty{0,0};Check(resource->Map(0,&empty,&mapped));std::memcpy(mapped,data,static_cast<size_t>(bytes));resource->Unmap(0,nullptr); }
        retained.push_back(resource);return resource;
    }
    template<class T> ComPtr<ID3D12Resource> Upload(const std::vector<T>& values) {
        return Buffer(values.size()*sizeof(T),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,values.data());
    }
    D3D12_GPU_DESCRIPTOR_HANDLE View(ID3D12Resource* resource,UINT index,UINT stride,UINT count,bool write) {
        auto cpu=views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=static_cast<SIZE_T>(index)*descriptorSize;
        auto handle=views->GetGPUDescriptorHandleForHeapStart();handle.ptr+=static_cast<UINT64>(index)*descriptorSize;
        if(write) {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};desc.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.NumElements=count;desc.Buffer.StructureByteStride=stride;
            device->CreateUnorderedAccessView(resource,nullptr,&desc,cpu);
        } else {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc{};desc.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Buffer.NumElements=count;desc.Buffer.StructureByteStride=stride;
            device->CreateShaderResourceView(resource,&desc,cpu);
        }
        return handle;
    }
    void Uav(ID3D12Resource* resource) {
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barrier.UAV.pResource=resource;
        list->ResourceBarrier(1,&barrier);
    }
    void Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
    }
    void Submit() {
        Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(fence.Get(),1));
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"Fence event creation failed");
        HRESULT hr=fence->SetEventOnCompletion(1,event);
        DWORD wait=FAILED(hr)?WAIT_FAILED:WaitForSingleObject(event,15000);CloseHandle(event);Check(hr);
        Require(wait==WAIT_OBJECT_0,"GPU did not complete within 15 seconds");Check(device->GetDeviceRemovedReason());
    }
};
static ComPtr<ID3D12RootSignature> Root(Device& gpu,bool ray) {
    D3D12_ROOT_PARAMETER p[4]{};
    D3D12_DESCRIPTOR_RANGE ranges[4]{};
    for(UINT i=0;i<4;++i) {
        ranges[i].RangeType=i==3?D3D12_DESCRIPTOR_RANGE_TYPE_UAV:D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[i].NumDescriptors=1;ranges[i].BaseShaderRegister=i==3?0:i;
        p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[i].DescriptorTable={1,&ranges[i]};
    }
    if(ray) {
        p[0]={};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;p[0].Descriptor={0,7};
        ranges[1].RegisterSpace=7;
        ranges[2].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[2].BaseShaderRegister=0;ranges[2].RegisterSpace=7;
        p[3]={};p[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[3].Constants={0,7,4};
    }
    D3D12_ROOT_SIGNATURE_DESC desc{4,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));return root;
}
static ComPtr<ID3D12Resource> Scene(Device& gpu) {
    // Opaque left half and alpha-tested right half of a plane between receiver
    // and lights. Dedicated hit records ensure cutout decisions stay per geometry.
    std::vector<float3> vertices={
        {-2,-4,2},{0,-4,2},{0,4,2},{-2,-4,2},{0,4,2},{-2,4,2},
        {0,-4,2},{2,-4,2},{2,4,2},{0,-4,2},{2,4,2},{0,4,2}};
    auto vb=gpu.Upload(vertices);
    D3D12_RAYTRACING_GEOMETRY_DESC geometry[2]{};
    for(UINT i=0;i<2;++i) {
        auto& g=geometry[i];g.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        g.Flags=i==0?D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE:D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        g.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;g.Triangles.VertexCount=6;
        g.Triangles.VertexBuffer={vb->GetGPUVirtualAddress()+i*6*sizeof(float3),sizeof(float3)};
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
    inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=2;inputs.pGeometryDescs=geometry;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO size{};gpu.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&size);
    Require(size.ResultDataMaxSizeInBytes>0,"BLAS prebuild returned no size");
    auto blas=gpu.Buffer(size.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,true);
    auto scratch=gpu.Buffer(size.ScratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.Inputs=inputs;build.DestAccelerationStructureData=blas->GetGPUVirtualAddress();build.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
    gpu.list->BuildRaytracingAccelerationStructure(&build,0,nullptr);gpu.Uav(blas.Get());
    D3D12_RAYTRACING_INSTANCE_DESC instance{};instance.Transform[0][0]=instance.Transform[1][1]=instance.Transform[2][2]=1;
    instance.InstanceMask=1;instance.AccelerationStructure=blas->GetGPUVirtualAddress();
    auto instances=gpu.Upload(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>{instance});
    inputs={};inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=1;inputs.InstanceDescs=instances->GetGPUVirtualAddress();
    gpu.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&size);Require(size.ResultDataMaxSizeInBytes>0,"TLAS prebuild returned no size");
    auto tlas=gpu.Buffer(size.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,true);
    scratch=gpu.Buffer(size.ScratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    build={};build.Inputs=inputs;build.DestAccelerationStructureData=tlas->GetGPUVirtualAddress();build.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
    gpu.list->BuildRaytracingAccelerationStructure(&build,0,nullptr);gpu.Uav(tlas.Get());return tlas;
}
static ShadowTask Task(uint receiver,uint light,float3 target) {
    ShadowTask t{};t.normal={0,0,1};t.originBias=0.0001f;t.endpointBias=0.0001f;
    t.lightCenter=target;t.lightId=light;t.sampleCount=1;t.instanceMask=1;t.receiverId=receiver;t.frameTag=19;return t;
}
int main() try {
    std::cout<<"Control per-light DXR backend test 1 (standalone; no game files modified)\n";
    Device gpu;auto scene=Scene(gpu);auto rayRoot=Root(gpu,true);auto computeRoot=Root(gpu,false);
    auto library=Read("rt_lights_visibility.dxil");
    D3D12_DXIL_LIBRARY_DESC lib{{library.data(),library.size()},0,nullptr};
    D3D12_HIT_GROUP_DESC opaque{L"opaque",D3D12_HIT_GROUP_TYPE_TRIANGLES,nullptr,L"blocked",nullptr};
    D3D12_HIT_GROUP_DESC cutout{L"cutout",D3D12_HIT_GROUP_TYPE_TRIANGLES,L"fixtureCutout",L"blocked",nullptr};
    D3D12_RAYTRACING_SHADER_CONFIG shader{4,8};D3D12_RAYTRACING_PIPELINE_CONFIG pipeline{1};
    ID3D12RootSignature* root=rayRoot.Get();
    D3D12_STATE_SUBOBJECT objects[]={
        {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&lib},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&opaque},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&cutout},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&shader},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipeline},
        {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&root}};
    D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,6,objects};
    ComPtr<ID3D12StateObject> state;Check(gpu.device->CreateStateObject(&desc,IID_PPV_ARGS(&state)));
    ComPtr<ID3D12StateObjectProperties> props;Check(state.As(&props));
    std::vector<unsigned char> table(256,0);
    const wchar_t* names[]={L"perLightVisibility",L"visible",L"opaque",L"cutout"};
    for(size_t i=0;i<4;++i) {const void* identifier=props->GetShaderIdentifier(names[i]);Require(identifier!=nullptr,"Missing shader identifier");std::memcpy(table.data()+i*64,identifier,32);}
    auto sbt=gpu.Upload(table);
    std::vector<ShadowTask> tasks={Task(0,10,{-2,0,4}),Task(0,11,{2,0,4}),Task(1,10,{-2,0,1}),Task(1,11,{2,0,4}),Task(2,10,{-2,0,4}),Task(2,11,{2,0,4}),Task(3,12,{0,0,4})};
    tasks[3].instanceMask=2;tasks[4].normal={0,0,0};tasks[6].emitterAxisU={2,0,0};tasks[6].emitterAxisV={0,2,0};tasks[6].sampleCount=32;
    auto input=gpu.Upload(tasks);
    auto shadows=gpu.Buffer(tasks.size()*sizeof(Visibility),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    gpu.list->SetComputeRootSignature(rayRoot.Get());gpu.list->SetComputeRootShaderResourceView(0,scene->GetGPUVirtualAddress());gpu.list->SetComputeRootDescriptorTable(1,gpu.View(input.Get(),0,sizeof(ShadowTask),static_cast<UINT>(tasks.size()),false));gpu.list->SetComputeRootDescriptorTable(2,gpu.View(shadows.Get(),1,sizeof(Visibility),static_cast<UINT>(tasks.size()),true));
    uint constants[]={0,1,0,0};gpu.list->SetComputeRoot32BitConstants(3,4,constants,0);gpu.list->SetPipelineState1(state.Get());
    D3D12_DISPATCH_RAYS_DESC rays{};auto start=sbt->GetGPUVirtualAddress();rays.RayGenerationShaderRecord={start,32};rays.MissShaderTable={start+64,32,32};rays.HitGroupTable={start+128,128,64};rays.Width=static_cast<UINT>(tasks.size());rays.Height=rays.Depth=1;
    gpu.list->DispatchRays(&rays);gpu.Transition(shadows.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    std::vector<Contribution> lights;
    for(uint i=0;i<static_cast<uint>(tasks.size());++i) {
        const auto& t=tasks[i];Contribution c{};c.visibilityIndex=i;c.receiverId=t.receiverId;c.lightId=t.lightId;c.frameTag=t.frameTag;
        c.diffuse=(i%2==0)?float3{2,0,0}:float3{0,0,3};c.specular=(i%2==0)?float3{1,0,0}:float3{0,0,1.5f};lights.push_back(c);
    }
    // Two deliberately invalid receivers: stale frame and out-of-range batch.
    Contribution stale=lights[1];stale.receiverId=4;stale.frameTag=18;lights.push_back(stale);
    std::vector<ReceiverRange> ranges={{0,2,19,0},{2,2,19,0},{4,2,19,0},{6,1,19,0},{7,1,19,0},{100,2,19,0}};
    auto lightBuffer=gpu.Upload(lights),rangeBuffer=gpu.Upload(ranges);
    auto output=gpu.Buffer(ranges.size()*sizeof(Lighting),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    auto cs=Read("rt_lights_compose.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC cp{};cp.pRootSignature=computeRoot.Get();cp.CS={cs.data(),cs.size()};ComPtr<ID3D12PipelineState> compose;Check(gpu.device->CreateComputePipelineState(&cp,IID_PPV_ARGS(&compose)));
    gpu.list->SetComputeRootSignature(computeRoot.Get());gpu.list->SetPipelineState(compose.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.View(shadows.Get(),2,sizeof(Visibility),static_cast<UINT>(tasks.size()),false));gpu.list->SetComputeRootDescriptorTable(1,gpu.View(lightBuffer.Get(),3,sizeof(Contribution),static_cast<UINT>(lights.size()),false));gpu.list->SetComputeRootDescriptorTable(2,gpu.View(rangeBuffer.Get(),4,sizeof(ReceiverRange),static_cast<UINT>(ranges.size()),false));gpu.list->SetComputeRootDescriptorTable(3,gpu.View(output.Get(),5,sizeof(Lighting),static_cast<UINT>(ranges.size()),true));gpu.list->Dispatch(1,1,1);
    gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.Transition(shadows.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto shadowRead=gpu.Buffer(tasks.size()*sizeof(Visibility),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    auto lightRead=gpu.Buffer(ranges.size()*sizeof(Lighting),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.list->CopyResource(shadowRead.Get(),shadows.Get());gpu.list->CopyResource(lightRead.Get(),output.Get());gpu.Submit();
    Visibility* v=nullptr;Lighting* l=nullptr;Check(shadowRead->Map(0,nullptr,reinterpret_cast<void**>(&v)));Check(lightRead->Map(0,nullptr,reinterpret_cast<void**>(&l)));
    for(size_t i=0;i<tasks.size();++i) {std::cout<<"task="<<i<<" light="<<v[i].lightId<<" receiver="<<v[i].receiverId<<" valid="<<v[i].valid<<" visibility="<<v[i].value<<'\n';Require(v[i].lightId==tasks[i].lightId && v[i].receiverId==tasks[i].receiverId && v[i].frameTag==19,"Result identity mismatch");}
    Require(v[0].valid==1 && v[0].value==0,"Opaque blocker not shadowed");Require(v[1].valid==1 && v[1].value==1,"Cutout incorrectly blocks the other light");
    Require(v[2].valid==1 && v[2].value==1,"Geometry beyond the light blocks it");Require(v[3].valid==1 && v[3].value==1,"Instance mask ignored");Require(v[4].valid==0,"Invalid geometry input accepted");
    Require(v[6].valid==1 && v[6].value>0.25f && v[6].value<0.75f,"Area-light edge lost partial visibility");
    Require(l[0].valid==1 && l[0].lightCount==2 && l[0].diffuse.x==0 && l[0].diffuse.z==3 && l[0].specular.x==0 && l[0].specular.z==1.5f,"Shadow leaked between overlapping lights");
    Require(l[1].valid==1 && l[1].diffuse.x==2 && l[1].diffuse.z==3,"Unoccluded light sum incorrect");
    Require(l[2].valid==0 && l[4].valid==0 && l[5].valid==0,"Incomplete/stale/out-of-range lighting accepted");
    Require(l[3].valid==1 && std::abs(l[3].diffuse.x-2*v[6].value)<1e-5f,"Soft visibility composition incorrect");
    shadowRead->Unmap(0,nullptr);lightRead->Unmap(0,nullptr);
    std::cout<<"PASS: hardware DXR per-light occlusion, cutout, endpoint, mask, soft edge, independent diffuse/specular composition and invalid-result rejection\n";
    return 0;
} catch(const std::exception& error) { std::cerr<<"FAIL: "<<error.what()<<'\n';return 1; }
