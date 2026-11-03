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
#include "../src/gi_reuse/evaluator_reference_pipeline.h"
using Microsoft::WRL::ComPtr;
void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned long>(h)));}
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<char> Read(const char* path){std::ifstream f(path,std::ios::binary);Require(bool(f),path);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv)try{
 Require(argc==3,"Expected native DXBC and translated DXIL paths");
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
 // Deliberately invalid cached bytes prove the replacement does not reuse them.
 const unsigned char invalidCache[]={1,2,3,4};desc.CachedPSO={invalidCache,sizeof(invalidCache)};
 ComPtr<ID3D12PipelineState> translated;
 Check(gi_reuse::CreateEvaluatorReferencePipeline(d.Get(),desc,{reference.data(),reference.size()},translated.GetAddressOf()));
 Require(desc.CS.pShaderBytecode==native.data()&&desc.CachedPSO.pCachedBlob==invalidCache,"Native descriptor modified");
 ID3D12PipelineState* invalid=original.Get();
 Require(gi_reuse::CreateEvaluatorReferencePipeline(d.Get(),desc,{},&invalid)==E_INVALIDARG&&!invalid,"Invalid shader did not fail closed");
 ComPtr<ID3D12InfoQueue> info;Check(d.As(&info));
 for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size=0;Check(info->GetMessage(i,nullptr,&size));std::vector<char> bytes(size);auto* msg=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(info->GetMessage(i,msg,&size));
  Require(msg->Severity!=D3D12_MESSAGE_SEVERITY_ERROR&&msg->Severity!=D3D12_MESSAGE_SEVERITY_CORRUPTION,msg->pDescription);
 }
 std::cout<<"PASS complete native and translated GI evaluator PSOs with captured native root; cache isolation and debug clean. No dispatch/image parity claim.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
