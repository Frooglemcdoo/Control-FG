#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "../src/pt_table_mirror.h"
using Microsoft::WRL::ComPtr;
static ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,UINT64 bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,const void* data=nullptr,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE){
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=type;ComPtr<ID3D12Resource> result;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&result))));
    if(data){void* mapped=nullptr;D3D12_RANGE none{};assert(SUCCEEDED(result->Map(0,&none,&mapped)));std::memcpy(mapped,data,static_cast<size_t>(bytes));result->Unmap(0,nullptr);}return result;
}
int main(){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    constexpr UINT prefix=244000;constexpr std::uint64_t mask=(1ull<<0)|(1ull<<5)|(1ull<<41);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;hd.NumDescriptors=prefix+147;
    ComPtr<ID3D12DescriptorHeap> native,cpu;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&native))));
    hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;hd.NumDescriptors=4;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu))));
    const UINT step=device->GetDescriptorHandleIncrementSize(hd.Type);
    const auto source=[&](UINT n){auto h=cpu->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(n)*step;return h;};
    const auto destination=[&](UINT n){auto h=native->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(n)*step;return h;};
    const auto gpu=[&](UINT n){auto h=native->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(n)*step;return h;};
    PTTableMirror::Bank bank;assert(bank.HeapCreated(device.Get(),native.Get()));assert(!bank.HeapCreated(device.Get(),native.Get()));
    PTTableMirror::Bank::Snapshot oldSnapshot,newSnapshot,refreshed;assert(!bank.Read(native.Get(),prefix,mask,oldSnapshot));
    std::array<UINT,128> constants{};constants[0]=constants[64]=11;constants[65]=1;
    const UINT valueA=42,valueB=100;
    auto a=Buffer(device.Get(),4,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,&valueA);
    auto b=Buffer(device.Get(),4,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,&valueB);
    auto cb=Buffer(device.Get(),sizeof(constants),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,constants.data());
    auto out=Buffer(device.Get(),8,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    auto readback=Buffer(device.Get(),8,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Buffer.NumElements=1;srv.Buffer.StructureByteStride=4;
    device->CreateShaderResourceView(a.Get(),&srv,source(0));device->CreateShaderResourceView(a.Get(),&srv,source(2));
    D3D12_CONSTANT_BUFFER_VIEW_DESC cv{cb->GetGPUVirtualAddress(),256};device->CreateConstantBufferView(&cv,source(1));
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;uav.Buffer.NumElements=2;uav.Buffer.StructureByteStride=4;
    device->CreateUnorderedAccessView(out.Get(),nullptr,&uav,source(3));
    // Copy begins outside the mirrored suffix: clipping must advance the source.
    device->CopyDescriptorsSimple(2,destination(prefix-1),source(0),hd.Type);bank.Copy(2,destination(prefix-1),source(0));
    D3D12_CPU_DESCRIPTOR_HANDLE to[]={destination(prefix+5),destination(prefix+41)},from=source(2);const UINT two=2;
    device->CopyDescriptors(2,to,nullptr,1,&from,&two,hd.Type);bank.Copy({{to[0],1},{to[1],1}},{{from,2}});
    assert(bank.Read(native.Get(),prefix,mask,oldSnapshot));assert(bank.Unchanged(oldSnapshot));
    assert(bank.Read(native.Get(),prefix,mask,refreshed));auto* retainedCpu=refreshed.descriptors.Get();
    device->CreateShaderResourceView(b.Get(),&srv,source(2));assert(bank.Unchanged(oldSnapshot)); // CPU source changed; native table did not.
    device->CreateShaderResourceView(b.Get(),&srv,destination(prefix+5));bank.ShaderResource(b.Get(),&srv,destination(prefix+5));
    cv.BufferLocation+=256;device->CreateConstantBufferView(&cv,destination(prefix));bank.Constant(&cv,destination(prefix));
    device->CreateUnorderedAccessView(out.Get(),nullptr,&uav,destination(prefix+41));bank.UnorderedAccess(out.Get(),nullptr,&uav,destination(prefix+41));
    assert(!bank.Unchanged(oldSnapshot));assert(bank.Read(native.Get(),prefix,mask,newSnapshot));assert(bank.Unchanged(newSnapshot));
    assert(bank.Refresh(refreshed)&&refreshed.descriptors.Get()==retainedCpu&&bank.Unchanged(refreshed));
    {
        PTTableMirror::Mute privateWrites;
        device->CopyDescriptorsSimple(49,destination(prefix+49),oldSnapshot.At(0),hd.Type);bank.Copy(49,destination(prefix+49),oldSnapshot.At(0));
        device->CopyDescriptorsSimple(49,destination(prefix+98),refreshed.At(0),hd.Type);bank.Copy(49,destination(prefix+98),refreshed.At(0));
    }
    PTTableMirror::Bank::Snapshot excluded;assert(!bank.Read(native.Get(),prefix+49,mask,excluded));
    bank.Invalidate();assert(!bank.Refresh(refreshed));assert(!bank.Unchanged(newSnapshot));assert(!bank.Read(native.Get(),prefix,mask,excluded));
    const char code[]="cbuffer C:register(b4){uint addend,index;};StructuredBuffer<uint> Input:register(t0);RWStructuredBuffer<uint> Output:register(u0);[numthreads(1,1,1)]void main(){Output[index]=Input[0]+addend;}";
    ComPtr<ID3DBlob> shader,error,signature;assert(SUCCEEDED(D3DCompile(code,sizeof(code)-1,nullptr,nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS,0,&shader,&error)));
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND}};
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable={3,ranges};
    D3D12_ROOT_SIGNATURE_DESC rd{1,&parameter,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};assert(SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1_0,&signature,&error)));
    ComPtr<ID3D12RootSignature> root;assert(SUCCEEDED(device->CreateRootSignature(0,signature->GetBufferPointer(),signature->GetBufferSize(),IID_PPV_ARGS(&root))));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};ComPtr<ID3D12PipelineState> pso;
    assert(SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso))));
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC qd{};assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pso.Get(),IID_PPV_ARGS(&list))));
    ID3D12DescriptorHeap* heaps[]={native.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());
    list->SetComputeRootDescriptorTable(0,gpu(prefix+49));list->Dispatch(1,1,1);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barrier.UAV.pResource=out.Get();list->ResourceBarrier(1,&barrier);
    list->SetComputeRootDescriptorTable(0,gpu(prefix+98));list->Dispatch(1,1,1);
    barrier={};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={out.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
    list->CopyResource(readback.Get(),out.Get());assert(SUCCEEDED(list->Close()));ID3D12CommandList* submitted[]={list.Get()};queue->ExecuteCommandLists(1,submitted);
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));assert(SUCCEEDED(queue->Signal(fence.Get(),1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);assert(SUCCEEDED(fence->SetEventOnCompletion(1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    UINT* values=nullptr;const D3D12_RANGE read{0,8};assert(SUCCEEDED(readback->Map(0,&read,reinterpret_cast<void**>(&values))));assert(values[0]==53&&values[1]==111);const D3D12_RANGE empty{};readback->Unmap(0,&empty);
    assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    puts("PASS: CPU-source descriptor mirroring, clipped/multi-range copies, direct views, immutable snapshots, private exclusion and WARP readback");
}
