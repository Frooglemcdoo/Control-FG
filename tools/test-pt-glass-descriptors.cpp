#include <windows.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3d12sdklayers.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../src/pt_glass_descriptors.h"
using Microsoft::WRL::ComPtr;
static ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,UINT64 bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,bool uav=false){
    D3D12_HEAP_PROPERTIES h{};h.Type=type;h.CreationNodeMask=h.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC r{};r.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;r.Width=bytes;r.Height=1;
    r.DepthOrArraySize=r.MipLevels=1;r.SampleDesc.Count=1;r.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if(uav)r.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ComPtr<ID3D12Resource> out;assert(SUCCEEDED(d->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&r,state,nullptr,IID_PPV_ARGS(&out))));return out;
}
int main(){
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory4> factory;assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    ComPtr<IDXGIAdapter> adapter;assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    ComPtr<ID3D12Device> device;assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    ComPtr<ID3D12InfoQueue> messages;device.As(&messages);
    constexpr UINT prefix=PTGlass::LightingTableLayout::NativeMaterialPrefix,nativeIndex=prefix,customIndex=prefix+49;
    const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.NumDescriptors=prefix+98;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> gpu;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&gpu))));
    hd.NumDescriptors=51;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ComPtr<ID3D12DescriptorHeap> cpu;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu))));
    auto ch=[&](UINT i){auto h=cpu->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*step;return h;};
    auto gh=[&](UINT i){auto h=gpu->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(i)*step;return h;};
    auto gd=[&](UINT i){auto h=gpu->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*step;return h;};
    auto constants=Buffer(device.Get(),512,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    void* mapped=nullptr;assert(SUCCEEDED(constants->Map(0,nullptr,&mapped)));memset(mapped,0,512);
    *static_cast<UINT*>(mapped)=10;*reinterpret_cast<UINT*>(static_cast<char*>(mapped)+256)=20;constants->Unmap(0,nullptr);
    auto inputs=Buffer(device.Get(),256,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    assert(SUCCEEDED(inputs->Map(0,nullptr,&mapped)));memset(mapped,0,256);
    auto* values=static_cast<UINT*>(mapped);values[0]=7;values[1]=3;values[2]=5;inputs->Unmap(0,nullptr);
    auto nativeOut=Buffer(device.Get(),16,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    auto customOut=Buffer(device.Get(),16,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    auto readback=Buffer(device.Get(),32,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_CONSTANT_BUFFER_VIEW_DESC cb{constants->GetGPUVirtualAddress(),256};
    for(UINT i=0;i<5;++i)device->CreateConstantBufferView(&cb,ch(i));
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Buffer.NumElements=1;srv.Buffer.StructureByteStride=4;
    srv.Buffer.FirstElement=1;
    for(UINT i=5;i<41;++i)device->CreateShaderResourceView(inputs.Get(),&srv,ch(i));
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;uav.Buffer.NumElements=4;uav.Buffer.StructureByteStride=4;
    for(UINT i=41;i<49;++i)device->CreateUnorderedAccessView(nativeOut.Get(),nullptr,&uav,ch(i));
    srv.Buffer.FirstElement=2;device->CreateShaderResourceView(inputs.Get(),&srv,ch(49));
    device->CreateUnorderedAccessView(customOut.Get(),nullptr,&uav,ch(50));
    device->CopyDescriptorsSimple(49,gd(nativeIndex),ch(0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    srv.Buffer.FirstElement=0;device->CreateShaderResourceView(inputs.Get(),&srv,gd(0));
    // Mirror from CPU sources only. Rejected attempts must not publish or write
    // the native table/material prefix, even with a fully populated snapshot.
    PTGlass::LightingTableShadow shadow;assert(SUCCEEDED(shadow.Initialize(device.Get())));
    assert(shadow.Begin(71,{gpu.Get(),nativeIndex,49}));
    assert(!shadow.Copy(71,0,{gpu.Get(),nativeIndex,49}));
    assert(!shadow.Copy(70,0,{cpu.Get(),0,49}));
    assert(shadow.Copy(71,0,{cpu.Get(),0,49}));assert(!shadow.Ready(71));
    cb.BufferLocation+=256;
    assert(shadow.ConfigureSecondary(71,cb,{cpu.Get(),49,1},{cpu.Get(),50,1}));
    D3D12_GPU_DESCRIPTOR_HANDLE custom{};
    assert(!shadow.Publish(71,{gpu.Get(),nativeIndex,49},&custom));
    assert(!shadow.Publish(71,{gpu.Get(),0,49},&custom));
    assert(shadow.Publish(71,{gpu.Get(),customIndex,49},&custom));
    assert(!shadow.Copy(71,0,{cpu.Get(),0,49}));
    assert(!shadow.Publish(71,{gpu.Get(),customIndex,49},&custom));
    custom=gh(customIndex);
    // Reconstruct the relevant native root ABI; root 7 addresses the material
    // prefix, root 2 is b0. A root constant selects the output lane in the test.
    D3D12_DESCRIPTOR_RANGE ranges[5]{};
    ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,0};
    ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,5};
    ranges[2]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,41};
    ranges[3]={D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,1,0,0,0};
    ranges[4]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,1,0};
    D3D12_ROOT_PARAMETER p[8]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[0].DescriptorTable={3,ranges};
    p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={9,0,1};
    for(UINT i=2;i<6;++i){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i].Descriptor={i-2,0};}
    p[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[6].DescriptorTable={1,&ranges[3]};
    p[7].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[7].DescriptorTable={1,&ranges[4]};
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=8;rd.pParameters=p;
    ComPtr<ID3DBlob> rootBytes,errors;assert(SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&rootBytes,&errors)));
    ComPtr<ID3D12RootSignature> root;assert(SUCCEEDED(device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root))));
    const char* shader="#ifndef BIAS\n#define BIAS 0\n#endif\ncbuffer A:register(b0){uint a;} cbuffer B:register(b6){uint b;} cbuffer C:register(b9){uint lane;} StructuredBuffer<uint> hit:register(t28); StructuredBuffer<uint> material:register(t0,space1); RWStructuredBuffer<uint> outp:register(u0); [numthreads(1,1,1)] void main(){outp[lane]=a+b+hit[0]+material[0]+BIAS;}";
    ComPtr<ID3DBlob> code;assert(SUCCEEDED(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"main","cs_5_1",0,0,&code,&errors)));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code->GetBufferPointer(),code->GetBufferSize()};
    ComPtr<ID3D12PipelineState> nativePso,customPso;assert(SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&nativePso))));
    const D3D_SHADER_MACRO customMacro[]={{"BIAS","100"},{nullptr,nullptr}};
    ComPtr<ID3DBlob> customCode;assert(SUCCEEDED(D3DCompile(shader,strlen(shader),nullptr,customMacro,nullptr,"main","cs_5_1",0,0,&customCode,&errors)));
    pd.CS={customCode->GetBufferPointer(),customCode->GetBufferSize()};
    assert(SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&customPso))));
    D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12CommandAllocator> allocator;assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    ComPtr<ID3D12GraphicsCommandList> list;assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nativePso.Get(),IID_PPV_ARGS(&list))));
    ID3D12DescriptorHeap* heaps[]={gpu.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());
    list->SetComputeRootDescriptorTable(0,gh(nativeIndex));list->SetComputeRootDescriptorTable(7,gh(0));
    list->SetComputeRootConstantBufferView(2,constants->GetGPUVirtualAddress());list->SetComputeRoot32BitConstant(1,0,0);
    list->Dispatch(1,1,1);
    {PTGlass::ScopedLightingBinding binding(list.Get(),nativePso.Get(),gh(nativeIndex),customPso.Get(),custom);list->Dispatch(1,1,1);}
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barrier.UAV.pResource=nativeOut.Get();list->ResourceBarrier(1,&barrier);
    list->SetComputeRoot32BitConstant(1,1,0);list->Dispatch(1,1,1);
    D3D12_RESOURCE_BARRIER transitions[2]{};
    for(UINT i=0;i<2;++i){transitions[i].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;transitions[i].Transition={i?customOut.Get():nativeOut.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};}
    list->ResourceBarrier(2,transitions);list->CopyBufferRegion(readback.Get(),0,nativeOut.Get(),0,16);list->CopyBufferRegion(readback.Get(),16,customOut.Get(),0,16);
    assert(SUCCEEDED(list->Close()));ID3D12CommandList* work[]={list.Get()};queue->ExecuteCommandLists(1,work);
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));assert(SUCCEEDED(queue->Signal(fence.Get(),1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);assert(SUCCEEDED(fence->SetEventOnCompletion(1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    assert(SUCCEEDED(readback->Map(0,nullptr,&mapped)));values=static_cast<UINT*>(mapped);
    assert(values[0]==30&&values[1]==30&&values[4]==142);readback->Unmap(0,nullptr);
    assert(shadow.Begin(72,{gpu.Get(),nativeIndex,49}));assert(!shadow.Ready(72));
    assert(!shadow.Copy(71,0,{cpu.Get(),0,49}));
    if(messages){for(UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T bytes=0;messages->GetMessage(i,nullptr,&bytes);auto* m=static_cast<D3D12_MESSAGE*>(malloc(bytes));assert(m);assert(SUCCEEDED(messages->GetMessage(i,m,&bytes)));if(m->Severity<=D3D12_MESSAGE_SEVERITY_ERROR){fputs(m->pDescription,stderr);abort();}free(m);}}
    puts("PASS: WARP native/custom/native values 30/142/30; material and root-CBV bindings preserved; CPU-only snapshots, overlap and stale epoch rejection.");
}
