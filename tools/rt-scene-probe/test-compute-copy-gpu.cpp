#include <windows.h>
#include "../../src/rt_descriptor_lease.h"
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
static rt_descriptor_lease::Snapshot leaseIdentity;
static rt_descriptor_lease::Block leaseBase;
static std::uint64_t leaseCursor=0,leaseAllocations=0;
static void Reserve(void*,std::uint64_t count,std::uint64_t* gpu,std::uint64_t* cpu,void** owner){++leaseAllocations;*gpu=leaseBase.gpu+leaseCursor*leaseIdentity.step;*cpu=leaseBase.cpu+leaseCursor*leaseIdentity.step;*owner=leaseBase.owner;leaseCursor+=count;}
static rt_descriptor_lease::Snapshot CurrentLease(){return leaseIdentity;}
static bool ValidLease(const rt_descriptor_lease::Block& block,const rt_descriptor_lease::Snapshot&){return block.owner==leaseBase.owner;}
int main(){
 ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&d)));
 constexpr unsigned N=8,Leased=32,Passes=3+Leased;const auto step=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=N+1;ComPtr<ID3D12DescriptorHeap> source;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&source)));hd.NumDescriptors=3*(N+1)+Leased*49;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ComPtr<ID3D12DescriptorHeap> destination;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&destination)));
 auto src=[&](unsigned i){return D3D12_CPU_DESCRIPTOR_HANDLE{source->GetCPUDescriptorHandleForHeapStart().ptr+std::size_t(i)*step};};auto dst=[&](unsigned i){return D3D12_CPU_DESCRIPTOR_HANDLE{destination->GetCPUDescriptorHandleForHeapStart().ptr+std::size_t(i)*step};};
 std::array<ComPtr<ID3D12Resource>,N> inputs;
 for(unsigned i=0;i<N;++i){inputs[i]=Buffer(d.Get(),D3D12_HEAP_TYPE_UPLOAD,256,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_GENERIC_READ);void* data=nullptr;Check(inputs[i]->Map(0,nullptr,&data));*static_cast<unsigned*>(data)=7*i+1;inputs[i]->Unmap(0,nullptr);D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Buffer.NumElements=1;v.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;d->CreateShaderResourceView(inputs[i].Get(),&v,src(i));}
 auto output=Buffer(d.Get(),D3D12_HEAP_TYPE_DEFAULT,N*4,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);auto readback=Buffer(d.Get(),D3D12_HEAP_TYPE_READBACK,Passes*N*4,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;u.Buffer.NumElements=N;u.Buffer.StructureByteStride=4;d->CreateUnorderedAccessView(output.Get(),nullptr,&u,src(N));
 for(unsigned i=0;i<N+1;++i)d->CopyDescriptorsSimple(1,dst(i),src(i),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 std::array<D3D12_CPU_DESCRIPTOR_HANDLE,N> ds{},ss{};for(unsigned i=0;i<N;++i){ds[i]=dst(N+1+i);ss[i]=src(i);}d->CopyDescriptors(N,ds.data(),nullptr,N,ss.data(),nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);d->CopyDescriptorsSimple(1,dst(2*N+1),src(N),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 // Negative control: a wrong source must change GPU output.
 for(unsigned i=0;i<N;++i)ds[i]=dst(2*(N+1)+i);ss[3]=src(4);d->CopyDescriptors(N,ds.data(),nullptr,N,ss.data(),nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);d->CopyDescriptorsSimple(1,dst(3*N+2),src(N),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 std::array<D3D12_GPU_DESCRIPTOR_HANDLE,Passes> gpuTables{};
 for(unsigned pass=0;pass<3;++pass)gpuTables[pass]={destination->GetGPUDescriptorHandleForHeapStart().ptr+UINT64(pass)*(N+1)*step};
 leaseIdentity={destination.Get(),d.Get(),source.Get(),1,1,step};
 leaseBase={destination->GetGPUDescriptorHandleForHeapStart().ptr+3*(N+1)*step,dst(3*(N+1)).ptr,destination.Get()};
 rt_descriptor_lease::Api leaseApi{Reserve,CurrentLease,ValidLease};rt_descriptor_lease::Lease lease;
 for(unsigned pass=0;pass<Leased;++pass){rt_descriptor_lease::Block block;rt_descriptor_lease::Take(leaseApi,lease,nullptr,49,block);gpuTables[3+pass]={block.gpu};
  for(unsigned i=0;i<N;++i)d->CopyDescriptorsSimple(1,{SIZE_T(block.cpu+i*step)},src((i+pass)%N),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  d->CopyDescriptorsSimple(1,{SIZE_T(block.cpu+N*step)},src(N),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 }
 assert(leaseAllocations==2&&leaseCursor==Leased*49);
 D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,N,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,N};D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;param.DescriptorTable={2,ranges};param.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&param;
 ComPtr<ID3DBlob> serialized,errors;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors));ComPtr<ID3D12RootSignature> root;Check(d->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)));
 const char shader[]="ByteAddressBuffer input[8]:register(t0);RWStructuredBuffer<uint> output:register(u0);[numthreads(1,1,1)]void main(){[unroll]for(uint i=0;i<8;++i)output[i]=input[i].Load(0);}";ComPtr<ID3DBlob> cs;Check(D3DCompile(shader,sizeof(shader),nullptr,nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS,0,&cs,&errors));D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={cs->GetBufferPointer(),cs->GetBufferSize()};ComPtr<ID3D12PipelineState> pipeline;Check(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline)));
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(d->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));ComPtr<ID3D12CommandAllocator> allocator;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pipeline.Get(),IID_PPV_ARGS(&list)));
 ID3D12DescriptorHeap* heaps[]={destination.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());
 for(unsigned pass=0;pass<Passes;++pass){if(pass)Transition(list.Get(),output.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);list->SetComputeRootDescriptorTable(0,gpuTables[pass]);list->Dispatch(1,1,1);Transition(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readback.Get(),pass*N*4,output.Get(),0,N*4);}
 Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);ComPtr<ID3D12Fence> fence;Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);Check(queue->Signal(fence.Get(),1));Check(fence->SetEventOnCompletion(1,event));assert(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
 void* data=nullptr;D3D12_RANGE range{0,Passes*N*4};Check(readback->Map(0,&range,&data));auto* values=static_cast<unsigned*>(data);for(unsigned i=0;i<N;++i){assert(values[i]==1+7*i&&values[N+i]==values[i]);assert(values[2*N+i]==1+7*(i==3?4:i));}for(unsigned pass=0;pass<Leased;++pass)for(unsigned i=0;i<N;++i)assert(values[(3+pass)*N+i]==1+7*((i+pass)%N));readback->Unmap(0,nullptr);
 std::puts("PASS WARP shader output: eight individual copies equal one batched copy; wrong-source negative control detected. 32 leased tables keep distinct GPU outputs until submission, using two reservations. Not an RTX performance benchmark.");
}
