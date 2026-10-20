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
static std::atomic<unsigned long long> presentCount{0};
static void Log(const char*, ...) {}
static bool PTReflectionSha256(const unsigned char*, size_t, char*) { return false; }
#define CONTROL_LIGHTING_NO_OVERLAY
#include "../src/native_lighting_live_runtime.h"
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr) { if(FAILED(hr)) throw std::runtime_error("D3D failure " + std::to_string(static_cast<unsigned long>(hr))); }
int main() try {
    using namespace native_lighting;
    ComPtr<IDXGIFactory4> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter; Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device; Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    InstallLiveDiagnostic(device.Get());
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
        SelectMode(Mode::DisableLightPattern);render(direct.Get(),15,reset);render(gi.Get(),22,reset);
        SelectMode(Mode::Native);render(direct.Get(),11,reset);render(gi.Get(),22,reset);
    }
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
    CloseHandle(event);std::cout<<"PASS: WARP live native/GI/direct switching through SetPipelineState and Reset; native restored without restart\n";return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
