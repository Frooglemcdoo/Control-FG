#pragma once
// Included inside native_lighting. One capture per process; allocations remain
// alive even after reset, replay or failed submission. No frame-count retirement.
namespace rt_spot_capture_gpu {
using Microsoft::WRL::ComPtr;
#include "../build/rt_spot_capture_inputs.h"
#include "../build/rt_spot_capture_table.h"
inline constexpr UINT kRayCount=rt_spot_capture::CaptureMaxRecords;
inline constexpr UINT kInputBytes=kRayCount*rt_spot_capture::CaptureRayBytes, kResultBytes=kRayCount*rt_spot_capture::CaptureResultBytes;
inline constexpr UINT kTableBytes=8*1024*1024, kReadbackBytes=kResultBytes+16;
inline const GUID kInputState={0x964c9021,0x414a,0x43d4,{0xa1,0x11,0x70,0,0,0,0,3}};
inline const GUID kTranslate={0x964c9021,0x414a,0x43d4,{0xa1,0x11,0x70,0,0,0,0,4}};
inline const GUID kClear={0x964c9021,0x414a,0x43d4,{0xa1,0x11,0x70,0,0,0,0,5}};
inline const GUID kInputIdentifier={0x964c9021,0x414a,0x43d4,{0xa1,0x11,0x70,0,0,0,0,6}};
struct TableControls {UINT count=0,bytes=0,version=1,reserved=0;unsigned char nativeIds[2][32]{},captureIds[2][32]{};};
static_assert(sizeof(TableControls)==144);
struct Resources {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12Resource> inputs,results,table,status,upload,readback;
    ComPtr<ID3D12DescriptorHeap> staging;
    ComPtr<ID3D12Fence> fence;
    UINT step=0;
    HRESULT Buffer(UINT bytes,D3D12_HEAP_TYPE heapType,D3D12_RESOURCE_STATES initial,ID3D12Resource** output) noexcept {
        D3D12_HEAP_PROPERTIES heap{};heap.Type=heapType;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;
        desc.Height=desc.DepthOrArraySize=desc.MipLevels=desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(heapType==D3D12_HEAP_TYPE_DEFAULT)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        return device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,initial,nullptr,IID_PPV_ARGS(output));
    }
    HRESULT Initialize(ID3D12Device* owner) noexcept {
        if(device)return E_UNEXPECTED;
        device=owner;
        HRESULT hr=Buffer(kInputBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,&inputs);
        if(SUCCEEDED(hr))hr=Buffer(kResultBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,&results);
        if(SUCCEEDED(hr))hr=Buffer(kTableBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,&table);
        if(SUCCEEDED(hr))hr=Buffer(16,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,&status);
        if(SUCCEEDED(hr))hr=Buffer(1024,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,&upload);
        if(SUCCEEDED(hr))hr=Buffer(kReadbackBytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,&readback);
        D3D12_DESCRIPTOR_HEAP_DESC desc{};desc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;desc.NumDescriptors=49*4;
        if(SUCCEEDED(hr))hr=owner->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&staging));
        if(SUCCEEDED(hr))hr=owner->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
        if(SUCCEEDED(hr))step=owner->GetDescriptorHandleIncrementSize(desc.Type);
        return hr;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE At(UINT index) const noexcept {
        auto result=staging->GetCPUDescriptorHandleForHeapStart();result.ptr+=SIZE_T(index)*step;return result;
    }
    void RawUav(ID3D12Resource* resource,UINT bytes,UINT slot) noexcept {
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};desc.Format=DXGI_FORMAT_R32_TYPELESS;
        desc.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;desc.Buffer.NumElements=bytes/4;desc.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
        device->CreateUnorderedAccessView(resource,nullptr,&desc,At(slot));
    }
    void RawSrv(ID3D12Resource* resource,UINT64 offset,UINT bytes,UINT slot) noexcept {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};desc.Format=DXGI_FORMAT_R32_TYPELESS;desc.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
        desc.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;desc.Buffer.FirstElement=offset/4;
        desc.Buffer.NumElements=bytes/4;desc.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
        device->CreateShaderResourceView(resource,&desc,At(slot));
    }
};
inline void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) noexcept {
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&barrier);
}
inline void UavBarrier(ID3D12GraphicsCommandList* list) noexcept {
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;list->ResourceBarrier(1,&barrier);
}
inline HRESULT PrepareInputPipeline(ID3D12Device5* device,ID3D12Object* owner,ID3D12RootSignature* root) noexcept {
    if constexpr (!rt_spot_capture::GpuCaptureEnabled) return E_NOTIMPL;
    D3D12_EXPORT_DESC entry{L"captureNativeInputs",nullptr,D3D12_EXPORT_FLAG_NONE};
    D3D12_DXIL_LIBRARY_DESC library{{kSpotCaptureInputsShader,sizeof(kSpotCaptureInputsShader)},1,&entry};
    D3D12_GLOBAL_ROOT_SIGNATURE global{root};
    D3D12_RAYTRACING_SHADER_CONFIG config{4,8};D3D12_RAYTRACING_PIPELINE_CONFIG pipeline{1};
    D3D12_STATE_SUBOBJECT objects[]={{D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library},
        {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&global},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipeline}};
    D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,4,objects};
    ComPtr<ID3D12StateObject> input;ComPtr<ID3D12StateObjectProperties> properties;
    HRESULT hr=rt_shadow_capture::originalState(device,&desc,IID_PPV_ARGS(&input));
    if(SUCCEEDED(hr))hr=input.As(&properties);
    const void* identifier=SUCCEEDED(hr)?properties->GetShaderIdentifier(L"captureNativeInputs"):nullptr;
    if(SUCCEEDED(hr) && !identifier)hr=E_FAIL;
    if(SUCCEEDED(hr))hr=owner->SetPrivateData(kInputIdentifier,32,identifier);
    if(SUCCEEDED(hr))hr=owner->SetPrivateDataInterface(kInputState,input.Get());
    return hr;
}
inline HRESULT PrepareTablePipelines(ID3D12Device* device,ID3D12Object* owner,ID3D12RootSignature* root) noexcept {
    if constexpr (!rt_spot_capture::GpuCaptureEnabled) return E_NOTIMPL;
    HRESULT hr=S_OK;
    D3D12_COMPUTE_PIPELINE_STATE_DESC compute{};compute.pRootSignature=root;
    ComPtr<ID3D12PipelineState> translate,clear;
    compute.CS={kSpotCaptureTranslateShader,sizeof(kSpotCaptureTranslateShader)};
    if(SUCCEEDED(hr))hr=device->CreateComputePipelineState(&compute,IID_PPV_ARGS(&translate));
    compute.CS={kSpotCaptureClearShader,sizeof(kSpotCaptureClearShader)};
    if(SUCCEEDED(hr))hr=device->CreateComputePipelineState(&compute,IID_PPV_ARGS(&clear));
    if(SUCCEEDED(hr))hr=owner->SetPrivateDataInterface(kTranslate,translate.Get());
    if(SUCCEEDED(hr))hr=owner->SetPrivateDataInterface(kClear,clear.Get());
    return hr;
}
inline HRESULT PreparePipeline(ID3D12Device5* device,ID3D12StateObject* owner,ID3D12RootSignature* root) noexcept {
    if constexpr (!rt_spot_capture::GpuCaptureEnabled) return E_NOTIMPL;
    HRESULT hr=PrepareInputPipeline(device,owner,root);
    if(SUCCEEDED(hr))hr=PrepareTablePipelines(device,owner,root);
    Trace("rt_hit_gpu_pipelines ready=%u hr=0x%08lX exact_native_inputs=1 grid=64",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr));
    return hr;
}
}
