#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <fstream>
#include <iterator>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
#include "../src/gi_reuse/evaluator_reference_pipeline.h"
#include "../src/gi_reuse/d3d12_producer_table.h"
#include "../src/gi_reuse/d3d12_producer_pipeline.h"
#include "../src/gi_reuse/d3d12_initial_gi_pipeline.h"
static bool PTReflectionSha256(const unsigned char* data,size_t size,char (&hex)[65]) noexcept {
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;unsigned char digest[32]{};
 bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
 if(ok)ok=BCryptHashData(hash,const_cast<unsigned char*>(data),static_cast<ULONG>(size),0)>=0;
 if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);
 if(ok)for(unsigned i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02x",digest[i]);return ok;
}
#define CONTROL_GI_LONG_SEQUENCE
namespace native_lighting {
using CreateCompute=HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_COMPUTE_PIPELINE_STATE_DESC*,REFIID,void**);
inline void Trace(const char*,...) noexcept {}
#include "../src/gi_evaluator_runtime.inl"
}
HRESULT STDMETHODCALLTYPE Create(ID3D12Device* d,const D3D12_COMPUTE_PIPELINE_STATE_DESC* p,REFIID iid,void** out){return d->CreateComputePipelineState(p,iid,out);}
using Microsoft::WRL::ComPtr;
void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned long>(h)));}
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<char> Read(const char* path){std::ifstream f(path,std::ios::binary);Require(bool(f),path);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv)try{
 Require(argc==3||argc==4,"Expected native DXBC, translated DXIL and optional producer-shading DXIL paths");
 auto native=Read(argv[1]),reference=Read(argv[2]);
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d)));
 // Reuse captured eight-parameter native layout; no diagnostic root parameters.
 D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,0,1,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,244000,0,1,UINT_MAX}};
 D3D12_ROOT_PARAMETER parameters[10]{};
 parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={3,ranges};
 parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,ranges+3};
 for(UINT i=2;i<6;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i].Descriptor={i-2,0};}
 parameters[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[6].DescriptorTable={1,ranges+4};
 parameters[7].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[7].DescriptorTable={1,ranges+5};
 parameters[8].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[8].Descriptor={0,2};
 parameters[9].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[9].Constants={0,2,8};
 constexpr UINT rootCount=8;
 D3D12_ROOT_SIGNATURE_DESC rd{rootCount,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();desc.CS={native.data(),native.size()};
 ComPtr<ID3D12PipelineState> original;Check(d->CreateComputePipelineState(&desc,IID_PPV_ARGS(&original)));
 if(argc==4){
  auto producer=Read(argv[3]);auto producerDesc=desc;
  producerDesc.CS={producer.data(),producer.size()};
  ComPtr<ID3D12PipelineState> producerPipeline;
  Check(d->CreateComputePipelineState(&producerDesc,IID_PPV_ARGS(&producerPipeline)));
  std::cout<<"PASS supplied producer compute PSO uses captured native root.\n";
 }
 // Exercise bounded tile allocation/publication using a CPU descriptor shadow.
 // This validates resource ownership and table boundaries, not DXR execution.
 {
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  hd.NumDescriptors=244300;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> visible;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&visible)));
  hd.NumDescriptors=27;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  ComPtr<ID3D12DescriptorHeap> cpu;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu)));
  auto h=cpu->GetCPUDescriptorHandleForHeapStart();const auto step=d->GetDescriptorHandleIncrementSize(hd.Type);
  D3D12_SHADER_RESOURCE_VIEW_DESC nullView{};nullView.Format=DXGI_FORMAT_R32_FLOAT;
  nullView.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;nullView.Texture2D.MipLevels=1;
  nullView.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  for(UINT i=0;i<27;++i){d->CreateShaderResourceView(nullptr,&nullView,h);h.ptr+=step;}
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=256;bd.Height=1;
  bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  ComPtr<ID3D12Resource> cb;Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&cb)));
  const PTGlass::DescriptorRange originalRange{visible.Get(),244000,49};
  PTGlass::LightingTableShadow shadow;Check(shadow.Initialize(d.Get()));Require(shadow.Begin(17,originalRange),"Shadow begin failed");
  gi_reuse::D3D12InitialGITile missing;
  Require(missing.Initialize(d.Get(),shadow,17,2,3,256,5,7,2,1,0,2,6,5)==E_INVALIDARG,"Incomplete native bindings accepted");
  Require(shadow.Copy(17,5,{cpu.Get(),0,27}),"CPU source shadow copy failed");
  Require(shadow.Constant(17,0,{cb->GetGPUVirtualAddress(),256}),"Native b4 shadow failed");
  gi_reuse::D3D12InitialGITile tile;
  Check(tile.Initialize(d.Get(),shadow,17,2,3,256,5,7,2,1,0,2,6,5));
  Require(tile.Pixels()==2&&tile.FirstPixel()==0&&tile.RetainedBytes()==5344,"Tile allocation accounting mismatch");
  std::array<PTGlass::DescriptorRange,4> fresh{};for(UINT i=0;i<4;++i)fresh[i]={visible.Get(),244049+i*49,49};
  auto bad=fresh;bad[1]=bad[0];Require(!tile.Publish(originalRange,bad),"Overlapping private tables accepted");
  bad=fresh;bad[0]=originalRange;Require(!tile.Publish(originalRange,bad),"Native table overwrite accepted");
  bad=fresh;bad[0].first=0;Require(!tile.Publish(originalRange,bad),"Material prefix overwrite accepted");
  Require(tile.Publish(originalRange,fresh),"Valid private publication failed");
  Require(!tile.Publish(originalRange,fresh),"Published tile was mutable");
  std::cout<<"PASS bounded GI tile: native descriptor coverage, immutable private tables, resource accounting and overlap rejection. No traversal dispatch claim.\n";
 }
 // Deliberately invalid cached bytes prove the replacement does not reuse them.
 const unsigned char invalidCache[]={1,2,3,4};desc.CachedPSO={invalidCache,sizeof(invalidCache)};
 ComPtr<ID3D12PipelineState> translated;
 Check(gi_reuse::CreateEvaluatorReferencePipeline(d.Get(),desc,{reference.data(),reference.size()},translated.GetAddressOf()));
 Require(desc.CS.pShaderBytecode==native.data()&&desc.CachedPSO.pCachedBlob==invalidCache,"Native descriptor modified");
 ID3D12PipelineState* invalid=original.Get();
 Require(gi_reuse::CreateEvaluatorReferencePipeline(d.Get(),desc,{},&invalid)==E_INVALIDARG&&!invalid,"Invalid shader did not fail closed");
 SetEnvironmentVariableA("CONTROLTOH_GI_EVALUATOR","1");
 namespace evaluator=native_lighting::gi_evaluator;
 evaluator::Observe(d.Get(),&desc,original.Get(),&Create);
 Require(evaluator::ready.load(),"Runtime failed to prepare exact native shader");
 auto* selected=evaluator::Select(original.Get());Require(selected!=nullptr,"Translated selection unavailable");
 ComPtr<ID3D12CommandAllocator> allocator;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
 evaluator::Bind(list.Get(),selected);evaluator::Dispatch(list.Get());selected->Release();
 Require(evaluator::lastReported.load()==2,"Translated dispatch attribution failed");
 evaluator::Toggle();Require(evaluator::Select(original.Get())==nullptr,"Native toggle did not restore original");
 evaluator::Bind(list.Get(),original.Get());evaluator::Dispatch(list.Get());Require(evaluator::lastReported.load()==5,"Native dispatch attribution failed");
 evaluator::Bind(list.Get(),nullptr);evaluator::Dispatch(list.Get());Require(evaluator::lastReported.load()==5,"Stale binding survived reset");
 evaluator::Toggle();Require(evaluator::Select(translated.Get())==nullptr,"Foreign PSO accepted");
 Check(list->Close());
 ComPtr<ID3D12InfoQueue> info;Check(d.As(&info));
 for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size=0;Check(info->GetMessage(i,nullptr,&size));std::vector<char> bytes(size);auto* msg=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(info->GetMessage(i,msg,&size));
  Require(msg->Severity!=D3D12_MESSAGE_SEVERITY_ERROR&&msg->Severity!=D3D12_MESSAGE_SEVERITY_CORRUPTION,msg->pDescription);
 }
 std::cout<<"PASS complete native and translated GI evaluator PSOs with captured native root; cache isolation, runtime selection/reset and debug clean. No dispatch/image parity claim.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
