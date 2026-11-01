#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <climits>
#include <string>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
#include "../src/gi_source_patches.generated.h"
static std::atomic<unsigned long long> presentCount{1};
static bool PTReflectionSha256(const unsigned char* data,size_t size,char (&hex)[65]) noexcept {
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;unsigned char digest[32]{};
 bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
 if(ok)ok=BCryptHashData(hash,const_cast<unsigned char*>(data),static_cast<ULONG>(size),0)>=0;
 if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);
 if(ok)for(unsigned i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02x",digest[i]);return ok;
}
namespace native_lighting {
inline std::string auditLog;
inline void Trace(const char* format,...) noexcept {
 char message[2048]{};va_list args;va_start(args,format);vsnprintf(message,sizeof(message),format,args);va_end(args);
 auditLog+=message;auditLog+='\n';
}
namespace rt_spot_capture_audit {
inline const GUID kRootBytes={0xd8139341,0x0c53,0x46b7,{0xb1,0x29,0x48,0x22,0x18,0x00,0x00,0x01}};
}
#include "../src/gi_binding_audit.inl"
}
using Microsoft::WRL::ComPtr;
void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("GPU validation HRESULT "+std::to_string(static_cast<unsigned long>(hr)));}
std::vector<unsigned char> Read(const std::string& name){std::ifstream f(name,std::ios::binary);if(!f)throw std::runtime_error(name);return {(std::istreambuf_iterator<char>(f)),{}};}
int main()try{
 SetEnvironmentVariableA("CONTROLTOH_GI_BIND_AUDIT","1");
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
 D3D12_DESCRIPTOR_RANGE ranges[]={
  {D3D12_DESCRIPTOR_RANGE_TYPE_CBV,16,0,0,0},
  {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,32,0,0,16},
  {D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,48},
  {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,UINT_MAX,0,1,56}};
 D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
 parameter.DescriptorTable={4,ranges};parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
 D3D12_STATIC_SAMPLER_DESC samplers[20]{};
 for(UINT i=0;i<20;++i){auto& s=samplers[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxLOD=D3D12_FLOAT32_MAX;s.ShaderRegister=i;s.RegisterSpace=1;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;}
 D3D12_ROOT_SIGNATURE_DESC rd{1,&parameter,20,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> serialized,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&error));
 ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)));
 Check(root->SetPrivateData(native_lighting::rt_spot_capture_audit::kRootBytes,static_cast<UINT>(serialized->GetBufferSize()),serialized->GetBufferPointer()));
 ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
 for(unsigned i=0;i<8;++i)for(unsigned mode=1;mode<=7;++mode){
  const auto prefix="build/gi-source-"+std::to_string(i);
  auto native=Read(prefix+".dxbc"),expected=Read(prefix+"-"+std::to_string(mode)+".dxbc");std::vector<unsigned char> result;
  const auto& patch=*native_lighting::kGISourcePatches[i][mode-1];
  if(!native_lighting::MakeDisabledContribution(patch,native.data(),native.size(),patch.nativeSha,result)||result!=expected)throw std::runtime_error("C++ patch differs from generator");
  for(const auto* bytes:{&native,&expected}){
   ComPtr<ID3DBlob> assembly;Check(D3DDisassemble(bytes->data(),bytes->size(),0,nullptr,&assembly));
   D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();desc.CS={bytes->data(),bytes->size()};
   ComPtr<ID3D12PipelineState> pipeline;Check(device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&pipeline)));
   if(mode==1){
    namespace audit=native_lighting::gi_binding_audit;
    audit::Observe(&desc,pipeline.Get());audit::Bind(list.Get(),pipeline.Get());
    const auto before=audit::dispatches[i].load();audit::Dispatch(list.Get(),320,90,1);
    if(audit::dispatches[i].load()!=before+(bytes==&native?1:0))throw std::runtime_error("GI dispatch identity attribution failed");
    audit::Bind(list.Get(),nullptr);const auto cleared=audit::dispatches[i].load();audit::Dispatch(list.Get(),320,90,1);
    if(audit::dispatches[i].load()!=cleared)throw std::runtime_error("Stale GI identity survived unbind");
   }
  }
  native[patch.edits[0].offset]^=1;
  if(native_lighting::MakeDisabledContribution(patch,native.data(),native.size(),patch.nativeSha,result))throw std::runtime_error("Changed instruction accepted");
 }
 for(unsigned i=0;i<8;++i)if(native_lighting::auditLog.find("GI_BIND_R273_ROOT_COMPLETE variant="+std::to_string(i))==std::string::npos)throw std::runtime_error("GI root audit incomplete");
 std::cout<<"PASS R273 root audit: eight native shader identities, serialized WARP root layouts, dispatch attribution, foreign PSO rejection and unbind\n";
 std::cout<<"PASS R270 all 56 GI source variants: exact C++ patches, instruction rejection, disassembly and WARP PSO creation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
