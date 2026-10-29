// Validate the actual shipped SM5.1 container and unchanged binding ABI on D3D12 WARP.
#include "gpu_test_device.h"
#include "../build/gi_motion_history.generated.h"
int main(){try{
 Device gpu(false);
 D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,5,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER params[4]{};
 for(unsigned i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;}
 for(unsigned i=0;i<2;++i){params[2+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[2+i].DescriptorTable={1,&ranges[i]};}
 D3D12_STATIC_SAMPLER_DESC samplers[2]{};
 for(auto& s:samplers){s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxAnisotropy=1;}
 samplers[0].ShaderRegister=6;samplers[1].ShaderRegister=11;samplers[1].RegisterSpace=1;
 D3D12_ROOT_SIGNATURE_DESC desc{4,params,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=root.Get();p.CS={kGIMotionHistoryShader,sizeof(kGIMotionHistoryShader)};
 ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&p,IID_PPV_ARGS(&pso)));
 std::cout<<"PASS actual GI adaptive shader D3D12 WARP pipeline creation and native binding ABI\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
