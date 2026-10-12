#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include "../src/pt_glass_diagnostics.h"
using Microsoft::WRL::ComPtr;
int main() {
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    ComPtr<ID3DBlob> code,errors,serialized;
    const HRESULT compile=D3DCompileFromFile(L"tools/pt-glass-temporal-test.hlsl",nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors)fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);
    assert(SUCCEEDED(compile));
    D3D12_ROOT_PARAMETER parameters[4]{};
    parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[0].Constants={6,0,8};
    for(UINT i=1;i<4;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[i].Descriptor.ShaderRegister=i-1;}
    D3D12_ROOT_SIGNATURE_DESC rs{};rs.NumParameters=4;rs.pParameters=parameters;
    assert(SUCCEEDED(D3D12SerializeRootSignature(&rs,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors)));
    ComPtr<ID3D12RootSignature> root;
    assert(SUCCEEDED(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root))));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code->GetBufferPointer(),code->GetBufferSize()};
    ComPtr<ID3D12PipelineState> pso;assert(SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso))));
    auto buffer=[&](UINT64 bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
        D3D12_HEAP_PROPERTIES hp{};hp.Type=type;hp.CreationNodeMask=hp.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;
        d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(type==D3D12_HEAP_TYPE_DEFAULT)d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ComPtr<ID3D12Resource> r;assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r))));return r;
    };
    auto history=buffer(48,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto result=buffer(37*16,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto readback=buffer(37*16,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    auto diagnostics=buffer(37*128,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto diagnosticReadback=buffer(37*128,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;
    assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pso.Get(),IID_PPV_ARGS(&list))));
    list->SetComputeRootSignature(root.Get());
    list->SetComputeRootUnorderedAccessView(1,result->GetGPUVirtualAddress());
    list->SetComputeRootUnorderedAccessView(2,history->GetGPUVirtualAddress());
    list->SetComputeRootUnorderedAccessView(3,diagnostics->GetGPUVirtualAddress());
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
    for(UINT epoch=1;epoch<=37;++epoch){
        UINT previous=epoch-1,test=0;
        if(epoch>=33&&epoch<=35)test=epoch-32;
        if(epoch==36)previous=999;
        if(epoch==37)previous=0;
        float light=epoch<=32?(epoch%2?2.0f:4.0f):3.0f;UINT bits=0;memcpy(&bits,&light,4);
        UINT constants[8]{epoch,0,1,1,2,previous,test,bits};
        list->SetComputeRoot32BitConstants(0,8,constants,0);list->Dispatch(1,1,1);list->ResourceBarrier(1,&barrier);
    }
    barrier={};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={result.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};
    list->ResourceBarrier(1,&barrier);list->CopyResource(readback.Get(),result.Get());
    barrier.Transition.pResource=diagnostics.Get();list->ResourceBarrier(1,&barrier);
    list->CopyResource(diagnosticReadback.Get(),diagnostics.Get());assert(SUCCEEDED(list->Close()));
    ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));
    assert(SUCCEEDED(queue->Signal(fence.Get(),1)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);
    assert(SUCCEEDED(fence->SetEventOnCompletion(1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    void* mapped=nullptr;assert(SUCCEEDED(readback->Map(0,nullptr,&mapped)));const float* values=static_cast<const float*>(mapped);
    for(UINT i=0;i<37;++i)printf("history step=%u light=%.6f count=%.1f\n",i+1,values[i*4],values[i*4+3]);
    fflush(stdout);
    assert(values[0]==1.0f&&values[3]==1.0f); // Fresh history is not displayed.
    assert(std::abs(values[7*4]-3.0f)<0.001f&&values[7*4+3]==8.0f);
    assert(std::abs(values[31*4]-3.0f)<0.08f&&values[31*4+3]==16.0f);
    for(UINT i=32;i<37;++i)assert(values[i*4]==1.0f&&values[i*4+3]==1.0f);
    void* diagnosticData=nullptr;assert(SUCCEEDED(diagnosticReadback->Map(0,nullptr,&diagnosticData)));
    const auto* records=static_cast<const PTGlass::Diagnostic*>(diagnosticData);
    for(UINT i=0;i<37;++i){
        PTGlass::DiagnosticSummary s;s.Add(records[i],i+1,2);
        assert(s.sampled==1&&s.shaded==1&&s.diffuse==1&&s.invalid==0);
        assert(s.count==values[i*4+3]);assert(std::abs(s.resolved-values[i*4])<0.0001);
        assert(s.accepted==(i>0&&i<32?1u:0u));assert(s.warm==(i>=7&&i<32?1u:0u));
        assert(s.point[0]==2&&s.point[1]==1&&s.point[2]==1.25&&s.point[3]==3);
        assert(s.spot[0]==4&&s.spot[1]==2&&s.spot[2]==2.5&&s.spot[3]==3);
        assert(s.delta==-0.125&&s.absoluteDelta==0.25&&s.changed==1);
        PTGlass::DiagnosticSummary stale;stale.Add(records[i],999,2);assert(stale.sampled==0);
    }
    diagnosticReadback->Unmap(0,nullptr);
    puts("PASS: production GPU diagnostic stores, integer serials, 128-byte stride, disjoint layers, fence readback, history acceptance and shadow-summary decoding.");
    readback->Unmap(0,nullptr);
    puts("PASS: production temporal HLSL executed on WARP; warmup, noise reduction, history cap, position/normal/material/serial rejection and explicit reset.");
}
