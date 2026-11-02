#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <string>
#include <cstring>
#include <fstream>
#include <iterator>
#include "../src/gi_reuse/d3d12_raw_snapshot.h"
#include "../src/gi_reuse/d3d12_submission_metadata.h"
namespace fixture {
inline bool PatchSlot(void** slot,void* expected,void* replacement) noexcept {
 DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&old))return false;
 const bool ok=InterlockedCompareExchangePointer(slot,replacement,expected)==expected;
 DWORD ignored=0;VirtualProtect(slot,sizeof(void*),old,&ignored);return ok;
}
#include "../src/gi_compute_hooks.inl"
}
using Microsoft::WRL::ComPtr;using namespace gi_reuse;
void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned long>(h)));}
void Require(bool b,const char* text){if(!b)throw std::runtime_error(text);}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,UINT64 size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=type;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=size;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=flags;
 ComPtr<ID3D12Resource> r;Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&r)));return r;
}
unsigned refusedResets=0, indirectForwards=0;
HRESULT STDMETHODCALLTYPE RefusedReset(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*) {++refusedResets;return E_FAIL;}
void STDMETHODCALLTYPE ObservedIndirect(ID3D12GraphicsCommandList*,ID3D12CommandSignature*,UINT count,ID3D12Resource*,UINT64 offset,ID3D12Resource*,UINT64 countOffset) {
 Require(count==7 && offset==16 && countOffset==32,"indirect arguments preserved");++indirectForwards;
}
int main(int argc, char** argv) try {
 Require(argc == 2, "usage: test shader.dxil");
 SetEnvironmentVariableA("CONTROLTOH_GI_METADATA","1"); SetEnvironmentVariableA("CONTROLTOH_GI_BINDINGS","1");
 ComPtr<ID3D12Debug> debug; Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))); debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> warp; Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
 ComPtr<ID3D12Device> d; Check(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d)));
 std::ifstream file(argv[1], std::ios::binary); Require(file.good(), "shader file");
 std::vector<char> shader((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
 RawSnapshotPipeline snapshot; Check(snapshot.Initialize(d.Get(), shader.data(), shader.size()));
 // A different root layout and pipeline detect incomplete tracked restoration.
 D3D12_ROOT_PARAMETER params[5]{};
 D3D12_DESCRIPTOR_RANGE tableRange{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,1,0,0};
 params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
 params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; params[1].Constants = {0,0,4};
 params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
 params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; params[3].Descriptor.ShaderRegister=1;
 params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; params[4].DescriptorTable={1,&tableRange};
 D3D12_ROOT_SIGNATURE_DESC desc{5,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> blob, errors; Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors));
 ComPtr<ID3D12RootSignature> nativeRoot; Check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&nativeRoot)));
 const char sentinelCode[] = "RWByteAddressBuffer target:register(u0); ByteAddressBuffer raw:register(t0); ByteAddressBuffer table:register(t1); cbuffer C:register(b0){uint value; uint unused; uint extra; uint last;} cbuffer D:register(b1){uint fromCbv;} [numthreads(1,1,1)] void main(){target.Store(0,value+extra+last+raw.Load(0)+table.Load(0)+fromCbv);}";
 ComPtr<ID3DBlob> sentinelShader; Check(D3DCompile(sentinelCode,sizeof(sentinelCode)-1,nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&sentinelShader,&errors));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{}; pd.pRootSignature=nativeRoot.Get(); pd.CS={sentinelShader->GetBufferPointer(),sentinelShader->GetBufferSize()};
 ComPtr<ID3D12PipelineState> nativePipeline; Check(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&nativePipeline)));
 auto upload=Buffer(d.Get(),512,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE);
 auto source=Buffer(d.Get(),512,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE);
 auto output=Buffer(d.Get(),512,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 auto sentinel=Buffer(d.Get(),4,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 auto read=Buffer(d.Get(),516,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE);
 void* mapped=nullptr; D3D12_RANGE none{0,0}; Check(upload->Map(0,&none,&mapped));
 for(UINT i=0;i<512;++i) static_cast<unsigned char*>(mapped)[i]=static_cast<unsigned char>((i*17+39)%251);
 UINT sourceWord=0, tableWord=0;
 memcpy(&sourceWord,static_cast<unsigned char*>(mapped)+256,4); memcpy(&tableWord,mapped,4);
 upload->Unmap(0,nullptr);
 D3D12_DESCRIPTOR_HEAP_DESC hd{}; hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.NumDescriptors=1; hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
 ComPtr<ID3D12DescriptorHeap> heap; Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));
 D3D12_SHADER_RESOURCE_VIEW_DESC srv{}; srv.Format=DXGI_FORMAT_R32_TYPELESS; srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER; srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; srv.Buffer.NumElements=128; srv.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
 d->CreateShaderResourceView(upload.Get(),&srv,heap->GetCPUDescriptorHandleForHeapStart());
 D3D12_COMMAND_QUEUE_DESC qd{}; ComPtr<ID3D12CommandQueue> queue; Check(d->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator; Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList4> list; Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
 auto transition=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);};
 list->CopyBufferRegion(source.Get(),0,upload.Get(),0,512);
 list->CopyBufferRegion(output.Get(),0,upload.Get(),0,512);
 // Combined native read state stays untouched by the snapshot.
 transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_INDEX_BUFFER);
 transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 fixture::gi_compute_hooks::InstallList(list.Get());
 ComputeBindingSnapshot unobserved; UINT64 epoch=0;
 Require(!fixture::gi_compute_hooks::Capture(list.Get(),unobserved,epoch),"late attachment is not a known recording");
 fixture::gi_compute_hooks::Begin(list.Get(),nativePipeline.Get());
 Require(!fixture::gi_compute_hooks::failed.load(),"binding hook coverage");
 ID3D12DescriptorHeap* nativeHeaps[]={heap.Get()}; list->SetDescriptorHeaps(1,nativeHeaps);
 list->SetComputeRootSignature(nativeRoot.Get());
 list->SetPipelineState(nativePipeline.Get());
 list->SetComputeRootUnorderedAccessView(0,sentinel->GetGPUVirtualAddress());
 const UINT value=0x13579bdf, extras[]={7,11};
 list->SetComputeRoot32BitConstant(1,value,0);
 list->SetComputeRoot32BitConstants(1,2,extras,2);
 list->SetComputeRootShaderResourceView(2,upload->GetGPUVirtualAddress()+256);
 list->SetComputeRootConstantBufferView(3,upload->GetGPUVirtualAddress());
 list->SetComputeRootDescriptorTable(4,heap->GetGPUDescriptorHandleForHeapStart());
 ComputeBindingSnapshot captured; Require(fixture::gi_compute_hooks::Capture(list.Get(),captured,epoch),"capture intercepted compute bindings");
 const auto capturedEpoch=epoch;
 fixture::gi_compute_hooks::Read(list.Get())->tracker.Invalidate(); ComputeBindingSnapshot rejected; Require(!fixture::gi_compute_hooks::Capture(list.Get(),rejected,epoch),"reject invalidated recording");
 RawSnapshotPlan plan;
 Require(!snapshot.RecordRestoring(list.Get(),source.Get(),258,250,output.Get(),rejected,plan),"reject missing bindings without dispatch");
 Require(!snapshot.Record(list.Get(),source.Get(),510,4,output.Get(),plan),"reject overrun before recording");
 Require(!snapshot.Record(list.Get(),source.Get(),0,4,source.Get(),plan),"reject same-resource write");
 {fixture::gi_compute_hooks::Mute mute;
 Require(snapshot.RecordRestoring(list.Get(),source.Get(),258,250,output.Get(),captured,plan),"record uint16-offset snapshot");}
 Require(!fixture::gi_compute_hooks::Read(list.Get())->tracker.Ready(),"injected calls must not repair or overwrite invalidated native state");
 Require(plan.sourceOffset==256 && plan.prefixBytes==2 && plan.words==63,"aligned range");
 list->Dispatch(1,1,1);
 transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 transition(sentinel.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 list->CopyBufferRegion(read.Get(),0,output.Get(),0,512); list->CopyBufferRegion(read.Get(),512,sentinel.Get(),0,4);
 Check(list->Close()); ID3D12CommandList* lists[]={list.Get()}; queue->ExecuteCommandLists(1,lists);
 ComPtr<ID3D12Fence> fence; Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr); Require(event!=nullptr,"event");
 Check(queue->Signal(fence.Get(),1)); Check(fence->SetEventOnCompletion(1,event)); Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"GPU timeout"); CloseHandle(event);
 D3D12_RANGE range{0,516}; Check(read->Map(0,&range,&mapped));
 for(UINT i=0;i<512;++i){const UINT index=i<plan.copyBytes?256+i:i;Require(static_cast<unsigned char*>(mapped)[i]==static_cast<unsigned char>((index*17+39)%251),"copied bytes or untouched dispatch guard");}
 UINT restored=0; memcpy(&restored,static_cast<unsigned char*>(mapped)+512,4); Require(restored==value+extras[0]+extras[1]+sourceWord+tableWord+tableWord,"restored root/PSO/UAV/constants"); read->Unmap(0,&none);
 // Successful Reset advances generation and discards prior root arguments.
 Check(allocator->Reset()); Check(list->Reset(allocator.Get(),nativePipeline.Get()));
 Require(fixture::gi_compute_hooks::Read(list.Get())->generation>capturedEpoch,"reset generation");
 Require(!fixture::gi_compute_hooks::Capture(list.Get(),unobserved,epoch),"reset clears root state");
 list->SetComputeRootSignature(nativeRoot.Get());
 Require(fixture::gi_compute_hooks::Capture(list.Get(),unobserved,epoch),"reset initial PSO tracked");
 list->ClearState(nativePipeline.Get());
 Require(!fixture::gi_compute_hooks::Capture(list.Get(),unobserved,epoch),"clear state clears root");
 list->SetComputeRootSignature(nativeRoot.Get());
 const auto beforeFailure=fixture::gi_compute_hooks::Read(list.Get())->generation;
 // Stub only the saved forwarding functions: test refusal/invalidation without
 // issuing an invalid Reset or indirect dispatch to the real D3D12 runtime.
 auto* table=*reinterpret_cast<void***>(list.Get());
 fixture::gi_compute_hooks::Hooks* intercepted=nullptr;
 for(auto& h:fixture::gi_compute_hooks::hooks)if(h.table==table)intercepted=&h;
 Require(intercepted!=nullptr,"installed forwarding slot");
 const auto originalReset=intercepted->reset; intercepted->reset=&RefusedReset;
 const auto refused=list->Reset(allocator.Get(),nativePipeline.Get()); intercepted->reset=originalReset;
 Require(refused==E_FAIL && refusedResets==1,"failed reset forwarded once");
 Require(fixture::gi_compute_hooks::Read(list.Get())->generation==beforeFailure,"failed reset keeps generation");
 Require(fixture::gi_compute_hooks::Capture(list.Get(),unobserved,epoch),"failed reset preserves tracked state");
 const auto originalIndirect=intercepted->indirect; intercepted->indirect=&ObservedIndirect;
 list->ExecuteIndirect(nullptr,7,nullptr,16,nullptr,32); intercepted->indirect=originalIndirect;
 Require(indirectForwards==1 && !fixture::gi_compute_hooks::Capture(list.Get(),unobserved,epoch),"indirect invalidates tracked state");
 list->ClearState(nativePipeline.Get());list->SetComputeRootSignature(nativeRoot.Get());
 ComPtr<ID3D12CommandAllocator> bundleAllocator; Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE,IID_PPV_ARGS(&bundleAllocator)));
 ComPtr<ID3D12GraphicsCommandList> bundle; Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_BUNDLE,bundleAllocator.Get(),nullptr,IID_PPV_ARGS(&bundle)));
 Check(bundle->Close());list->ExecuteBundle(bundle.Get());
 Require(!fixture::gi_compute_hooks::Capture(list.Get(),unobserved,epoch),"bundle invalidates tracked state");
 Check(list->Close());
 ComPtr<ID3D12InfoQueue> messages; Check(d.As(&messages));
 for(UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T n=0;Check(messages->GetMessage(i,nullptr,&n));std::vector<unsigned char> storage(n);auto* m=reinterpret_cast<D3D12_MESSAGE*>(storage.data());Check(messages->GetMessage(i,m,&n));Require(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR,m->pDescription);}
 list.Reset();
 Require(fixture::gi_compute_hooks::liveStates.load()==0,"command-list destruction releases tracker");
 std::cout<<"PASS live binding hooks: real COM setters, late attachment refusal, mute, reset success/failure, ClearState, indirect forwarding and bundle invalidation\n";
 std::cout<<"PASS GPU raw snapshot: uint16 offset, source unchanged state, guard bytes, tracked compute restore: PSO/root/table/CBV/SRV/UAV/sparse constants, debug layer clean\n";
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
