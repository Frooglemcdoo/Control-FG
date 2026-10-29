// Validate the actual shipped SM5.1 container and unchanged binding ABI on D3D12 WARP.
#include "gpu_test_device.h"
static void Log(const char*,...) {}
#include "../src/rr_clamp_strength.h"
static ID3D12PipelineState* observed=nullptr;
static ID3D12PipelineState* expected=nullptr;
static unsigned dispatched=0,sets=0;
static bool throwDispatch=false;
static control_rr_clamp::SetFn realSet=nullptr;
static void STDMETHODCALLTYPE TrackSet(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){observed=pso;++sets;realSet(list,pso);}
static void STDMETHODCALLTYPE StubDispatch(ID3D12GraphicsCommandList*,UINT x,UINT y,UINT z){
 Require(x==3&&y==4&&z==1,"dispatch dimensions changed");Require(observed==expected,"wrong pipeline at dispatch");++dispatched;
 if(throwDispatch)RaiseException(0xE0420003,0,0,nullptr);
}
static ID3D12GraphicsCommandList* currentList=nullptr;
static ID3D12PipelineState* currentNative=nullptr;
static void NativeDispatch(void*,const void*){currentList->SetPipelineState(currentNative);currentList->Dispatch(3,4,1);}
static bool InvokeCatching(bool& caught){
 __try {return control_rr_clamp::InvokeGI(NativeDispatch,nullptr,nullptr,currentList);}
 __except(EXCEPTION_EXECUTE_HANDLER){caught=true;return false;}
}
int main(){try{
 Device gpu(false);
 D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,5,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER params[4]{};
 for(unsigned i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;}
 for(unsigned i=0;i<2;++i){params[2+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[2+i].DescriptorTable={1,&ranges[i]};}
 D3D12_STATIC_SAMPLER_DESC samplers[2]{};
 for(auto& s:samplers){s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxAnisotropy=1;}
 samplers[0].ShaderRegister=6;samplers[0].RegisterSpace=1;samplers[1].ShaderRegister=11;samplers[1].RegisterSpace=1;
 D3D12_ROOT_SIGNATURE_DESC desc{4,params,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=root.Get();p.CS={kGIMotionHistoryShader,sizeof(kGIMotionHistoryShader)};
 ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&p,IID_PPV_ARGS(&pso)));
 // Exercise the production creation observer with a real native PSO.
 p.CS={kGINativeHistoryShader,sizeof(kGINativeHistoryShader)};
 ComPtr<ID3D12PipelineState> native;Check(gpu.device->CreateComputePipelineState(&p,IID_PPV_ARGS(&native)));
 auto create=+[](ID3D12Device* d,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,REFIID iid,void** out)->HRESULT {
  Require(!desc->CachedPSO.pCachedBlob&&desc->CachedPSO.CachedBlobSizeInBytes==0,"cached PSO not cleared");
  return d->CreateComputePipelineState(desc,iid,out);
 };
 control_rr_gi::Capture(gpu.device.Get(),&p,native.Get(),create);
 auto* bundle=control_rr_gi::Find(native.Get());Require(bundle&&bundle->adaptive,"adaptive pipeline missing");
 control_rr_gi::Capture(gpu.device.Get(),&p,native.Get(),create);Require(control_rr_gi::ready.load()==1,"duplicate capture");
 p.CS={kGIMotionHistoryShader,sizeof(kGIMotionHistoryShader)};
 control_rr_gi::Capture(gpu.device.Get(),&p,pso.Get(),create);Require(control_rr_gi::ready.load()==1,"unknown shader captured");
 // Use the real scoped hooks with a recording dispatch callback, not an unbound GPU dispatch.
 Require(control_rr_clamp::InstallList(gpu.list.Get()),"list hook installation");
 realSet=control_rr_clamp::setOriginal;control_rr_clamp::setOriginal=&TrackSet;control_rr_clamp::dispatchOriginal=&StubDispatch;
 currentList=gpu.list.Get();currentNative=native.Get();expected=bundle->adaptive;
 Require(control_rr_clamp::InvokeGI(NativeDispatch,nullptr,nullptr,currentList),"GI scope not applied");
 Require(dispatched==1&&observed==native.Get()&&!control_rr_clamp::active,"native PSO or scope not restored");
 currentNative=pso.Get();expected=pso.Get();
 Require(!control_rr_clamp::InvokeGI(NativeDispatch,nullptr,nullptr,currentList),"unknown PSO should fall back");
 Require(dispatched==2&&observed==pso.Get(),"fallback dispatch missing");
 currentNative=native.Get();expected=bundle->adaptive;throwDispatch=true;bool caught=false;
 InvokeCatching(caught);Require(caught&&observed==native.Get()&&!control_rr_clamp::active,"exception restoration failed");
 std::cout<<"PASS R211 actual native/adaptive WARP PSOs, exact-source capture, duplicate rejection, production dispatch scope, native fallback and exception restoration\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
