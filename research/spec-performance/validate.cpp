#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
using Microsoft::WRL::ComPtr;
static bool Check(HRESULT hr,const char* label) {if(FAILED(hr)){printf("FAIL %s: %08lx\n",label,hr);return false;}return true;}
#include "execute-fixture.h"
int main(int argc,char**argv){try{
 if(argc!=2 && argc!=3)return 1;
 std::ifstream input(argv[1],std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(input)),{});
 ComPtr<ID3D12ShaderReflection> reflection;
 if(!Check(D3DReflect(bytes.data(),bytes.size(),IID_PPV_ARGS(&reflection)),"reflection"))return 2;
 D3D12_SHADER_DESC shader{};reflection->GetDesc(&shader);
 std::vector<D3D12_DESCRIPTOR_RANGE> ranges(shader.BoundResources);
 std::vector<D3D12_ROOT_PARAMETER> parameters(shader.BoundResources);
 for(UINT i=0;i<shader.BoundResources;++i){
  D3D12_SHADER_INPUT_BIND_DESC binding{};reflection->GetResourceBindingDesc(i,&binding);
  auto& range=ranges[i];range.RangeType=binding.Type==D3D_SIT_CBUFFER?D3D12_DESCRIPTOR_RANGE_TYPE_CBV:binding.Type==D3D_SIT_SAMPLER?D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:binding.Type==D3D_SIT_UAV_RWTYPED?D3D12_DESCRIPTOR_RANGE_TYPE_UAV:D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  range.NumDescriptors=binding.BindCount?binding.BindCount:UINT_MAX;
  range.BaseShaderRegister=binding.BindPoint;range.RegisterSpace=binding.Space;
  parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[i].DescriptorTable={1,&range};parameters[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
 }
 D3D12_ROOT_SIGNATURE_DESC desc{UINT(parameters.size()),parameters.data(),0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> serialized,error;
 auto hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&error);
 if(error)printf("%s\n",static_cast<const char*>(error->GetBufferPointer()));
 if(!Check(hr,"root serialize"))return 3;
 ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
 if(!Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"factory")||!Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"warp")||!Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)),"device"))return 4;
 ComPtr<ID3D12RootSignature> root;
 if(!Check(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)),"root"))return 5;
 D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root.Get();pipeline.CS={bytes.data(),bytes.size()};
 ComPtr<ID3D12PipelineState> pso;
 if(!Check(device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&pso)),"compute PSO"))return 6;
 printf("PASS reflection, compatible root signature and WARP compute PSO: %s\n",argv[1]);
 if(argc==3){
  std::ifstream other(argv[2],std::ios::binary);std::vector<char> parallel((std::istreambuf_iterator<char>(other)),{});
  pipeline.CS={parallel.data(),parallel.size()};ComPtr<ID3D12PipelineState> alternative;Require(device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&alternative)));
  for(UINT count:{0u,1u,2u,4u,8u,12u,16u})for(float clamp:{-1.f,2.5f}){
   auto a=ExecuteFixture(device.Get(),root.Get(),pso.Get(),reflection.Get(),false,13,19,count,clamp);
   auto b=ExecuteFixture(device.Get(),root.Get(),alternative.Get(),reflection.Get(),true,13,19,count,clamp);
   if(a!=b)throw std::runtime_error("fixture pixels differ: count="+std::to_string(count)+" clamp="+std::to_string(clamp));
   bool nonzero=false;for(auto word:a)nonzero|=word!=0;if(count && !nonzero)throw std::runtime_error("vacuous all-zero fixture");
   printf("PASS bitwise GPU equality: samples=%u clamp=%g partial edges and sentinel lanes\n",count,clamp);
  }
 }
 return 0;
}catch(const std::exception& e){printf("FAIL %s\n",e.what());return 10;}}

