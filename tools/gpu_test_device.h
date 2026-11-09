#pragma once
// Standalone GPU fixtures only. Never use blocking Submit() in the game runtime.
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
#include <string>
#include <vector>
using Microsoft::WRL::ComPtr;
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
    explicit Device(bool hardwareDxr=true) {
        ComPtr<IDXGIFactory6> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        if(!hardwareDxr) {
            ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
            Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)));
        }
        for(UINT i=0;hardwareDxr;++i) {
            ComPtr<IDXGIAdapter1> adapter;
            const HRESULT found=factory->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter));
            if(found==DXGI_ERROR_NOT_FOUND) break;
            Check(found);
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
        heap.NumDescriptors=64;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
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
