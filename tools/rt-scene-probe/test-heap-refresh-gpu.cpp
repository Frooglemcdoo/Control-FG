#include <windows.h>
#include "../../src/rt_owned_assembly.h"
#include <vector>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cassert>
#include <cstdio>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr){assert(SUCCEEDED(hr));}
static ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,D3D12_HEAP_TYPE heap,UINT64 bytes,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state){
 D3D12_HEAP_PROPERTIES h{};h.Type=heap;D3D12_RESOURCE_DESC r{};r.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;r.Width=bytes;r.Height=1;r.DepthOrArraySize=1;r.MipLevels=1;r.SampleDesc.Count=1;r.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;r.Flags=flags;
 ComPtr<ID3D12Resource> result;Check(d->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&r,state,nullptr,IID_PPV_ARGS(&result)));return result;
}
static void Transition(ID3D12GraphicsCommandList* c,ID3D12Resource* r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};c->ResourceBarrier(1,&b);}
static HMODULE verifiedD3d=nullptr;static const char* kD3dHash="";
static bool HashMatches(HMODULE,const char*){return false;}
static void* AllocateExecutableRelayNear(void*){return nullptr;}
static void Log(const char*,...){}
#include "../../src/rt_heap_refresh_runtime.h"
int main(){
 ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&d)));
 constexpr unsigned N=1024,Inputs=8,Passes=3;const auto step=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=N+1;ComPtr<ID3D12DescriptorHeap> source;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&source)));hd.NumDescriptors=3*(N+1);hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ComPtr<ID3D12DescriptorHeap> destination;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&destination)));
 auto src=[&](unsigned i){return D3D12_CPU_DESCRIPTOR_HANDLE{source->GetCPUDescriptorHandleForHeapStart().ptr+std::size_t(i)*step};};auto dst=[&](unsigned i){return D3D12_CPU_DESCRIPTOR_HANDLE{destination->GetCPUDescriptorHandleForHeapStart().ptr+std::size_t(i)*step};};
 std::array<ComPtr<ID3D12Resource>,Inputs> inputs;
 for(unsigned i=0;i<Inputs;++i){inputs[i]=Buffer(d.Get(),D3D12_HEAP_TYPE_UPLOAD,256,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_GENERIC_READ);void* data=nullptr;Check(inputs[i]->Map(0,nullptr,&data));*static_cast<unsigned*>(data)=7*i+1;inputs[i]->Unmap(0,nullptr);}
 D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=DXGI_FORMAT_R32_TYPELESS;view.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Buffer.NumElements=1;view.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
 for(unsigned i=0;i<N;++i)d->CreateShaderResourceView(inputs[i%Inputs].Get(),&view,src(i));
 auto output=Buffer(d.Get(),D3D12_HEAP_TYPE_DEFAULT,N*4,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);auto readback=Buffer(d.Get(),D3D12_HEAP_TYPE_READBACK,Passes*N*4,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;u.Buffer.NumElements=N;u.Buffer.StructureByteStride=4;d->CreateUnorderedAccessView(output.Get(),nullptr,&u,src(N));
 std::vector<unsigned char> module(0x112000),manager(0x1f8+N*8);std::array<unsigned char,0x40> heapA{},heapB{};
 using rt_owned_assembly::Write;
 RTHeapRefresh::base=module.data();RTHeapRefresh::ready=true;Write(module.data(),0x111bb8,manager.data());Write(module.data(),0x111bc0,step);Write(manager.data(),0x10,N);
 for(unsigned i=0;i<N;++i)Write(manager.data(),0x1f8+i*8,src(i).ptr);
 // Execute an actual seven-byte vtable call with all eight Win64 arguments.
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(code);
 const unsigned char callBody[]={
  0x48,0x83,0xec,0x48,
  0x48,0x8b,0x44,0x24,0x70,0x48,0x89,0x44,0x24,0x20,
  0x48,0x8b,0x44,0x24,0x78,0x48,0x89,0x44,0x24,0x28,
  0x48,0x8b,0x84,0x24,0x80,0,0,0,0x48,0x89,0x44,0x24,0x30,
  0x48,0x8b,0x84,0x24,0x88,0,0,0,0x48,0x89,0x44,0x24,0x38,
  0x4c,0x8b,0x11,0x41,0xff,0x92,0xb8,0,0,0,0x48,0x83,0xc4,0x48,0xc3};
 std::memcpy(code,callBody,sizeof(callBody));
 const unsigned char setter[]={0x4c,0x89,0x84,0xd1,0xf8,1,0,0,0x48,0xff,0x41,8,0xc3};std::memcpy(code+128,setter,sizeof(setter));
 assert(FlushInstructionCache(GetCurrentProcess(),code,4096));
 auto copyCall=reinterpret_cast<decltype(&RTHeapRefresh::Copy)>(code);auto publishCall=reinterpret_cast<decltype(&RTHeapRefresh::Publish)>(code+128);
 auto nativeDest=dst(0);UINT nativeSize=N;
 copyCall(d.Get(),1,&nativeDest,&nativeSize,N,reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(manager.data()+0x1f8),nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 publishCall(manager.data(),300,src(300).ptr);assert(rt_owned_assembly::Read<std::uint64_t>(manager.data(),8)==1);
 // Near relays within the same allocation, identical E8/E9 encoding as runtime.
 auto patch=[&](unsigned offset,unsigned size,void* hook,bool jump){
  auto* relay=code+256+offset;const unsigned char stub[]={0xff,0x25,0,0,0,0};std::memcpy(relay,stub,6);std::memcpy(relay+6,&hook,8);
  unsigned char call[6]{};assert(rt_allocator_probe::Call6(reinterpret_cast<std::uintptr_t>(code+offset),reinterpret_cast<std::uintptr_t>(relay),call));
  RTHeapRefresh::Patch target{};target.site=code+offset;target.size=size;std::memcpy(target.before,target.site,size);std::memset(target.after,0x90,size);std::memcpy(target.after,call,5);if(jump)target.after[0]=0xe9;
  assert(RTHeapRefresh::Exchange(target,true));
 };
 patch(53,7,reinterpret_cast<void*>(&RTHeapRefresh::Copy),false);patch(128,13,reinterpret_cast<void*>(&RTHeapRefresh::Publish),true);
 assert(FlushInstructionCache(GetCurrentProcess(),code,4096));
 auto refresh=[&](auto& heap,unsigned offset){
  Write(manager.data(),0x60,heap.data());Write(heap.data(),0,destination.Get());Write(heap.data(),8,dst(offset).ptr);
  auto destinationStart=dst(offset);UINT count=N;
  copyCall(d.Get(),1,&destinationStart,&count,N,reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(manager.data()+0x1f8),nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  Write(heap.data(),0x30,rt_owned_assembly::Read<std::uint64_t>(manager.data(),8));Write(heap.data(),0x38,N);
 };
 refresh(heapA,0);refresh(heapB,N+1);
 // Rewrite the existing CPU descriptor in place, then republish the SAME handle.
 d->CreateShaderResourceView(inputs[7].Get(),&view,src(300));
 publishCall(manager.data(),300,src(300).ptr);
 refresh(heapB,N+1);refresh(heapB,N+1);
 assert(RTHeapRefresh::fullCopies==2&&RTHeapRefresh::partialCopies==1&&RTHeapRefresh::skips==1);
 assert(RTHeapRefresh::copiedDescriptors==2*N+256&&RTHeapRefresh::fallbacks==0);
 auto reference=dst(2*(N+1));UINT count=N;d->CopyDescriptors(1,&reference,&count,N,reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(manager.data()+0x1f8),nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 std::array<D3D12_GPU_DESCRIPTOR_HANDLE,Passes> gpuTables{};
 for(unsigned pass=0;pass<Passes;++pass){gpuTables[pass]={destination->GetGPUDescriptorHandleForHeapStart().ptr+UINT64(pass)*(N+1)*step};d->CopyDescriptorsSimple(1,dst(pass*(N+1)+N),src(N),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
 D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,N,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,N};D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;param.DescriptorTable={2,ranges};param.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&param;
 ComPtr<ID3DBlob> serialized,errors;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors));ComPtr<ID3D12RootSignature> root;Check(d->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)));
 const char shader[]="ByteAddressBuffer input[1024]:register(t0);RWStructuredBuffer<uint> output:register(u0);[numthreads(32,1,1)]void main(uint3 id:SV_DispatchThreadID){output[id.x]=input[NonUniformResourceIndex(id.x)].Load(0);}";ComPtr<ID3DBlob> cs;Check(D3DCompile(shader,sizeof(shader),nullptr,nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS,0,&cs,&errors));D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={cs->GetBufferPointer(),cs->GetBufferSize()};ComPtr<ID3D12PipelineState> pipeline;Check(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline)));
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(d->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));ComPtr<ID3D12CommandAllocator> allocator;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pipeline.Get(),IID_PPV_ARGS(&list)));
 ID3D12DescriptorHeap* heaps[]={destination.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());
 for(unsigned pass=0;pass<Passes;++pass){if(pass)Transition(list.Get(),output.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);list->SetComputeRootDescriptorTable(0,gpuTables[pass]);list->Dispatch(N/32,1,1);Transition(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readback.Get(),pass*N*4,output.Get(),0,N*4);}
 Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);ComPtr<ID3D12Fence> fence;Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);Check(queue->Signal(fence.Get(),1));Check(fence->SetEventOnCompletion(1,event));assert(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
 void* data=nullptr;D3D12_RANGE range{0,Passes*N*4};Check(readback->Map(0,&range,&data));auto* values=static_cast<unsigned*>(data);for(unsigned i=0;i<N;++i){assert(values[i]==1+7*(i%Inputs));assert(values[N+i]==1+7*(i==300?7:i%Inputs));assert(values[2*N+i]==values[N+i]);}readback->Unmap(0,nullptr);
 assert(VirtualFree(code,0,MEM_RELEASE));
 std::puts("PASS WARP: incremental refresh equals full-copy GPU output across 1024 descriptors; same-handle republish updates changed slot, unchanged pages remain intact, repeated refresh skips copy.");
}
