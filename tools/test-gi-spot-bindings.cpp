#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "../build/gi_spot_trace.generated.h"
#include "../src/native_lighting_report.h"
namespace native_lighting {
inline std::atomic<unsigned long long> presentCount{0};
inline SRWLOCK reportLock=SRWLOCK_INIT;
inline SessionReport report;
inline bool Trace(const char*,...) noexcept{return true;}
// This test exercises real command-list hooks and restoration, not PSO matching.
inline bool PTReflectionSha256(const unsigned char*,size_t,char*) noexcept{return false;}
using CreateCompute=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_COMPUTE_PIPELINE_STATE_DESC*,REFIID,void**);
using SetPso=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using Dispatch=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
struct Native{SetPso set;Dispatch dispatch;};
inline Native Lookup(ID3D12GraphicsCommandList* list) noexcept{auto** v=*reinterpret_cast<void***>(list);return {reinterpret_cast<SetPso>(v[25]),reinterpret_cast<Dispatch>(v[14])};}
inline bool PatchSlot(void** slot,void* expected,void* hook) noexcept{DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&old))return false;const bool ok=InterlockedCompareExchangePointer(slot,hook,expected)==expected;DWORD ignored=0;VirtualProtect(slot,sizeof(void*),old,&ignored);return ok;}
namespace rt_spot_capture_audit {inline const GUID kRootBytes={};}
#include "../src/gi_spot_trace_runtime.inl"
}
using Microsoft::WRL::ComPtr;
void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("D3D12 call failed");}
void Require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main()try{
 SetEnvironmentVariableA("CONTROLTOH_GI_SPOT_TRACE","1");
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d)));
 D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,5},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,41},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,0,1,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,244000,0,1,0}};
 D3D12_ROOT_PARAMETER p[9]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[0].DescriptorTable={3,ranges};p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={1,ranges+3};
 for(UINT i=2;i<6;++i){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i].Descriptor={i-2,0};}
 p[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[6].DescriptorTable={1,ranges+4};p[7].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[7].DescriptorTable={1,ranges+5};p[8].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[8].Descriptor={0,2};
 ComPtr<ID3D12RootSignature> root,extended;for(UINT n=8;n<10;++n){D3D12_ROOT_SIGNATURE_DESC rd{n,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> built;Check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&built)));if(n==8)root=built;else extended=built;}
 ComPtr<ID3D12DescriptorHeap> descriptors,samplers;D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,244049,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&descriptors)));hd={D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,68,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&samplers)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
 namespace probe=native_lighting::gi_spot_trace;probe::InstallList(list.Get());Require(probe::phase.load()!=probe::Phase::Failed,"hook install");
 ID3D12DescriptorHeap* heaps[]={descriptors.Get(),samplers.Get()};list->SetDescriptorHeaps(2,heaps);list->SetComputeRootSignature(root.Get());
 auto set=[&] {for(UINT i=0;i<8;++i){if(i>=2&&i<=5)list->SetComputeRootConstantBufferView(i,0x10000ull*i);else list->SetComputeRootDescriptorTable(i,(i==1||i==6)?samplers->GetGPUDescriptorHandleForHeapStart():descriptors->GetGPUDescriptorHandleForHeapStart());}};
 set();auto s=probe::Read(list.Get());Require(s.mask==255&&s.root==root.Get(),"root arguments not observed");
 for(UINT i=2;i<6;++i)Require(s.values[i]==0x10000ull*i,"CBV root slot");
 list->SetComputeRootSignature(root.Get());Require(probe::Read(list.Get()).mask==255,"same-root state incorrectly discarded");list->SetDescriptorHeaps(2,heaps);Require(probe::Read(list.Get()).mask==255,"same-heap state incorrectly discarded");
 auto hooks=probe::LookupHooks(list.Get());probe::Rebind(list.Get(),hooks,extended.Get(),s);probe::Rebind(list.Get(),hooks,root.Get(),s);auto restored=probe::Read(list.Get());Require(s.mask==restored.mask&&s.root==restored.root&&std::equal(s.values,s.values+8,restored.values),"internal rebinding changed native observation");
 list->SetComputeRootSignature(extended.Get());Require(probe::Read(list.Get()).mask==0,"root change failed to invalidate");list->SetComputeRootSignature(root.Get());set();
 list->SetDescriptorHeaps(0,nullptr);Require((probe::Read(list.Get()).mask&0xc3)==0,"heap change failed to invalidate tables");list->SetDescriptorHeaps(2,heaps);set();
 ComPtr<ID3D12CommandAllocator> ba;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE,IID_PPV_ARGS(&ba)));ComPtr<ID3D12GraphicsCommandList> bundle;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_BUNDLE,ba.Get(),nullptr,IID_PPV_ARGS(&bundle)));Check(bundle->Close());list->ExecuteBundle(bundle.Get());Require(probe::Read(list.Get()).mask==0&&probe::Read(list.Get()).root==nullptr,"bundle failed to invalidate");
 const auto generation=probe::Read(list.Get()).generation;probe::Reset(list.Get());Require(probe::Read(list.Get()).mask==0&&probe::Read(list.Get()).generation==generation+1,"reset generation");Check(list->Close());
 std::cout<<"PASS R274 production hooks: root/table/CBV slots, heap/root invalidation, duplicate bindings, private rebinding, bundle invalidation, reset generation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
