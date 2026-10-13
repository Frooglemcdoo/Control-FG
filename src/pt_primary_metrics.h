#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
// Sparse GPU coverage counters, sampled at one pixel per 16x16 block. Own these
// through the producer's native fence; Read is valid ONLY after that fence.
namespace PTPrimaryMetrics {
class Counters {
    Microsoft::WRL::ComPtr<ID3D12Resource> gpu,readback,upload;
    struct Copy {Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;};
    std::array<Copy,5> copies;bool enabled=false,cancelled=false;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu;
public:
    bool Initialize(ID3D12Device* device){
        if(!device||gpu)return false;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=128;desc.Height=1;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&gpu))))return false;
        hp.Type=D3D12_HEAP_TYPE_READBACK;desc.Flags=D3D12_RESOURCE_FLAG_NONE;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))))return false;
        hp.Type=D3D12_HEAP_TYPE_UPLOAD;desc.Width=136;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload))))return false;
        void* data=nullptr;const D3D12_RANGE empty{};
        if(FAILED(upload->Map(0,&empty,&data))||!data)return false;
        UINT initial[34]{};initial[32]=1;initial[33]=2;std::memcpy(data,initial,sizeof(initial));upload->Unmap(0,nullptr);
        for(unsigned i=0;i<copies.size();++i){auto& copy=copies[i];
            const auto type=(i==2||i==4)?D3D12_COMMAND_LIST_TYPE_COPY:D3D12_COMMAND_LIST_TYPE_DIRECT;
            if(FAILED(device->CreateCommandAllocator(type,IID_PPV_ARGS(&copy.allocator)))||
               FAILED(device->CreateCommandList(0,type,copy.allocator.Get(),nullptr,IID_PPV_ARGS(&copy.list))))return false;
            copy.list->CopyBufferRegion(gpu.Get(),32,upload.Get(),i?(i>=3?132:128):0,4);
            if(FAILED(copy.list->Close()))return false;
        }
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;
        if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu))))return false;
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32_TYPELESS;uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
        uav.Buffer.NumElements=32;uav.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;device->CreateUnorderedAccessView(gpu.Get(),nullptr,&uav,Uav());return true;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE Uav()const{return cpu?cpu->GetCPUDescriptorHandleForHeapStart():D3D12_CPU_DESCRIPTOR_HANDLE{};}
    ID3D12Resource* Buffer()const{return gpu.Get();}
    // Copy-queue admission is queued BEFORE the native signal that releases
    // the graphics consumer. Callers must prove that consumer is still blocked.
    // Immutable upload and command storage live through the consumer's fence.
    bool SetAdmission(ID3D12CommandQueue* queue,bool allow,bool probe=false){
        if(!gpu||!queue|| (allow&&(enabled||cancelled)))return false;
        const auto type=queue->GetDesc().Type;
        if(type!=D3D12_COMMAND_LIST_TYPE_DIRECT&&type!=D3D12_COMMAND_LIST_TYPE_COPY)return false;
        if(!allow){if(cancelled)return true;if(enabled&&type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;cancelled=true;if(!enabled)return true;}
        // Lists are prebuilt once; neither admission nor replay cancellation can
        // fail allocating GPU commands at the submission boundary.
        const unsigned index=allow?((type==D3D12_COMMAND_LIST_TYPE_COPY?2:1)+(probe?2:0)):0;
        ID3D12CommandList* lists[]={copies[index].list.Get()};queue->ExecuteCommandLists(1,lists);
        if(allow)enabled=true;return true;
    }
    void Begin(ID3D12GraphicsCommandList* list,D3D12_GPU_DESCRIPTOR_HANDLE){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={gpu.Get(),0,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST};list->ResourceBarrier(1,&b);
        // Preserve the separately ordered admission word at byte 32.
        list->CopyBufferRegion(gpu.Get(),0,upload.Get(),0,32);
        list->CopyBufferRegion(gpu.Get(),36,upload.Get(),0,92);
        b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;b.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;list->ResourceBarrier(1,&b);
    }
    void End(ID3D12GraphicsCommandList* list){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={gpu.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
        list->CopyBufferRegion(readback.Get(),0,gpu.Get(),0,128);
        b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COMMON;list->ResourceBarrier(1,&b);
    }
    bool ReadFaults(std::array<UINT,16>& values){
        values={};void* data=nullptr;const D3D12_RANGE read{36,100};
        if(!readback||FAILED(readback->Map(0,&read,&data))||!data)return false;
        std::memcpy(values.data(),static_cast<unsigned char*>(data)+36,64);const D3D12_RANGE noWrite{};readback->Unmap(0,&noWrite);return true;
    }
    bool Read(std::array<UINT,8>& values){
        values={};void* data=nullptr;const D3D12_RANGE read{0,32};
        if(!readback||FAILED(readback->Map(0,&read,&data))||!data)return false;
        std::memcpy(values.data(),data,32);const D3D12_RANGE noWrite{};readback->Unmap(0,&noWrite);return true;
    }
};
}
