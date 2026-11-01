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
#include <vector>
#include <cstring>
#include "../build/gi_spot_pcf.generated.h"
using Microsoft::WRL::ComPtr;
void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("GPU validation HRESULT "+std::to_string(static_cast<unsigned long>(hr)));}
std::vector<unsigned char> Read(const std::string& name){std::ifstream f(name,std::ios::binary);if(!f)throw std::runtime_error(name);return {(std::istreambuf_iterator<char>(f)),{}};}
int main()try{
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
 for(unsigned i=0;i<8;++i){
  const auto expected=Read("build/gi-spot-pcf-"+std::to_string(i)+".dxbc");
  const auto& shader=gi_spot_pcf::shaders[i];
  if(shader.size!=expected.size()||std::memcmp(shader.bytes,expected.data(),expected.size()))throw std::runtime_error("Embedded shader differs from generator");
  ComPtr<ID3DBlob> assembly;Check(D3DDisassemble(expected.data(),expected.size(),0,nullptr,&assembly));
  D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();desc.CS={shader.bytes,shader.size};
  ComPtr<ID3D12PipelineState> pipeline;Check(device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&pipeline)));
 }
 std::cout<<"PASS R268 all eight GI PCF shaders: embedded bytes, disassembly and WARP PSO creation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
