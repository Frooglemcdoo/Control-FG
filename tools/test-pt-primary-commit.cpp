#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include <array>
using Microsoft::WRL::ComPtr;
int main(){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    ComPtr<ID3DBlob> shader,error,serialized;
    HRESULT hr=D3DCompileFromFile(L"tools/test-pt-primary-commit.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&shader,&error);
    if(FAILED(hr)&&error)std::fprintf(stderr,"%s",static_cast<const char*>(error->GetBufferPointer()));assert(SUCCEEDED(hr));
    D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,0};
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable={1,&range};
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&parameter;
    assert(SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&error)));
    ComPtr<ID3D12RootSignature> root;assert(SUCCEEDED(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root))));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};
    ComPtr<ID3D12PipelineState> pso;assert(SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso))));
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap,cpu;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap))));
    hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu))));
    const UINT step=device->GetDescriptorHandleIncrementSize(hd.Type);
    std::array<ComPtr<ID3D12Resource>,2> textures;
    D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=8;td.Height=1;
    td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    for(UINT i=0;i<2;++i){assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&textures[i]))));
        D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=td.Format;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        auto ch=cpu->GetCPUDescriptorHandleForHeapStart();ch.ptr+=SIZE_T(i)*step;auto sh=heap->GetCPUDescriptorHandleForHeapStart();sh.ptr+=SIZE_T(i)*step;
        device->CreateUnorderedAccessView(textures[i].Get(),nullptr,&v,ch);device->CopyDescriptorsSimple(1,sh,ch,hd.Type);}
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=1024;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    hp.Type=D3D12_HEAP_TYPE_READBACK;ComPtr<ID3D12Resource> readback;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))));
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pso.Get(),IID_PPV_ARGS(&list))));
    ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);
    for(UINT i=0;i<2;++i){auto ch=cpu->GetCPUDescriptorHandleForHeapStart();ch.ptr+=SIZE_T(i)*step;
        auto gh=heap->GetGPUDescriptorHandleForHeapStart();gh.ptr+=UINT64(i)*step;const float initial[4]={2,3,4,2};
        list->ClearUnorderedAccessViewFloat(gh,ch,textures[i].Get(),initial,0,nullptr);}
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;list->ResourceBarrier(1,&barrier);
    list->SetComputeRootSignature(root.Get());list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());list->Dispatch(1,1,1);
    for(UINT i=0;i<2;++i){barrier={};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={textures[i].Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=textures[i].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint.Offset=512ull*i;
        dst.PlacedFootprint.Footprint={td.Format,8,1,1,256};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}
    assert(SUCCEEDED(list->Close()));ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));assert(SUCCEEDED(queue->Signal(fence.Get(),1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);assert(SUCCEEDED(fence->SetEventOnCompletion(1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    void* data=nullptr;D3D12_RANGE read{0,1024};assert(SUCCEEDED(readback->Map(0,&read,&data)));
    for(UINT i=0;i<2;++i){const auto* values=reinterpret_cast<const float*>(static_cast<const unsigned char*>(data)+512*i);
        for(UINT x=0;x<8;++x)for(UINT c=0;c<4;++c){const float initial[]={2,3,4,2},pt[]={.25f,.5f,.75f,.25f};
            const bool committed=x==3||x==6||x==7;const float expected=!committed?initial[c]:(i==0||x==6?0:pt[c]);assert(values[x*4+c]==expected);}}
    const D3D12_RANGE noWrite{};readback->Unmap(0,&noWrite);assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    puts("PASS: native light-buffer commit, RGBR encoding, rejected/probe/invalid/dimension fallback and zero radiance");
}
