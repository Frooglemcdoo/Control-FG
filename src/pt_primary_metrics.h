#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
// Sparse GPU coverage counters, sampled at one pixel per 16x16 block. Own these
// through the producer's native fence; Read is valid ONLY after that fence.
namespace PTPrimaryMetrics {
class Counters {
    Microsoft::WRL::ComPtr<ID3D12Resource> gpu,readback;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu;
public:
    bool Initialize(ID3D12Device* device){
        if(!device||gpu)return false;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=32;desc.Height=1;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&gpu))))return false;
        hp.Type=D3D12_HEAP_TYPE_READBACK;desc.Flags=D3D12_RESOURCE_FLAG_NONE;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))))return false;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;
        if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu))))return false;
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32_TYPELESS;uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
        uav.Buffer.NumElements=8;uav.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;device->CreateUnorderedAccessView(gpu.Get(),nullptr,&uav,Uav());return true;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE Uav()const{return cpu?cpu->GetCPUDescriptorHandleForHeapStart():D3D12_CPU_DESCRIPTOR_HANDLE{};}
    ID3D12Resource* Buffer()const{return gpu.Get();}
    void Begin(ID3D12GraphicsCommandList* list,D3D12_GPU_DESCRIPTOR_HANDLE descriptor){
        const UINT zero[4]{};list->ClearUnorderedAccessViewUint(descriptor,Uav(),gpu.Get(),zero,0,nullptr);
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=gpu.Get();list->ResourceBarrier(1,&b);
    }
    void End(ID3D12GraphicsCommandList* list){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={gpu.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
        list->CopyBufferRegion(readback.Get(),0,gpu.Get(),0,32);
        b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;list->ResourceBarrier(1,&b);
    }
    bool Read(std::array<UINT,8>& values){
        values={};void* data=nullptr;const D3D12_RANGE read{0,32};
        if(!readback||FAILED(readback->Map(0,&read,&data))||!data)return false;
        std::memcpy(values.data(),data,32);const D3D12_RANGE noWrite{};readback->Unmap(0,&noWrite);return true;
    }
};
}
