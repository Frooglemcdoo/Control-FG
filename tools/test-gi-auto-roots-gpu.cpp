#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_auto_evaluator.h"
#include "../src/gi_reuse/d3d12_auto_raygen.h"
int main()try {
 using namespace gi_reuse;Device d(false);
 D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,27,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,UINT_MAX,0,1,0}};
 D3D12_ROOT_PARAMETER p[8]{};
 for(UINT i=0;i<3;++i){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[i].DescriptorTable={1,ranges+i};}
 for(UINT i=0;i<5;++i){p[i+3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i+3].Descriptor={i,0};}
 D3D12_STATIC_SAMPLER_DESC samplers[4]{};const UINT registers[]={5,6,11,19};
 for(UINT i=0;i<4;++i){auto& s=samplers[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxLOD=D3D12_FLOAT32_MAX;s.ShaderRegister=registers[i];s.RegisterSpace=1;}
 D3D12_ROOT_SIGNATURE_DESC rd{8,p,4,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto evaluatorCode=Read("gi-auto-lighting.dxil");D3D12AutoEvaluator evaluator;
 Check(evaluator.Initialize(d.device.Get(),root.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),{evaluatorCode.data(),evaluatorCode.size()}));
 // These are layout admissions, not execution with fabricated native resources.
 ComputeBindingSnapshot missing;Require(!evaluator.Accept(missing),"missing root admitted");
 auto rayCode=Read("gi-auto-raygen.dxil");D3D12AutoRaygenAugment ray;
 Check(ray.Initialize(d.device.Get(),{rayCode.data(),rayCode.size()}));
 std::vector<D3D12_STATE_SUBOBJECT> objects;Require(!ray.Append(objects),"unreserved association admitted");objects.reserve(3);Require(ray.Append(objects),"local-root augmentation");
 const auto& assoc=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[2].pDesc);
 Require(assoc.pSubobjectToAssociate==objects.data()+1&&assoc.NumExports==1&&!wcscmp(assoc.pExports[0],D3D12AutoRaygenAugment::Export),"wrong local-root association");
 // Private-space overlap must fail before shader compilation or root mutation.
 p[3].Descriptor.RegisterSpace=19;blob.Reset();error.Reset();Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 D3D12AutoEvaluator collision;Require(collision.Initialize(d.device.Get(),root.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),{evaluatorCode.data(),evaluatorCode.size()})==E_INVALIDARG,"private-space collision admitted");
 D3D12_ROOT_PARAMETER large{};large.ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;large.Constants={0,0,60};
 rd={1,&large,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};blob.Reset();error.Reset();Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 D3D12AutoEvaluator excess;Require(excess.Initialize(d.device.Get(),root.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),{evaluatorCode.data(),evaluatorCode.size()})==E_INVALIDARG,"root DWORD overflow admitted");
 std::cout<<"PASS actual evaluator PSO root extension, private-space collision rejection, DWORD budget and raygen local-root association layout. Native DXR state-object linking not executed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
