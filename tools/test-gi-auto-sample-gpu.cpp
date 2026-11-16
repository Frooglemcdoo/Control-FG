#include "gpu_test_device.h"
#include "../src/gi_reuse/producer_sampling.h"
int main()try {
 Device d(false);constexpr UINT count=32768,scenarios=6,total=count*scenarios+scenarios*20;
 struct V{float x,y,z,w;};
 auto output=d.Buffer(UINT64(total)*sizeof(V),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto read=d.Buffer(UINT64(total)*sizeof(V),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_ROOT_PARAMETER p{};p.ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p.Descriptor={0,0};
 D3D12_ROOT_SIGNATURE_DESC rd{1,&p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-auto-sample.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootUnorderedAccessView(0,output->GetGPUVirtualAddress());d.list->Dispatch(count*scenarios/64,1,1);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyResource(read.Get(),output.Get());d.Submit();
 V* values=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&values)));
 for(UINT c=0;c<scenarios;++c){double z2=0,cap=0;
  for(UINT i=0;i<count;++i){const auto& v=values[c*count+i];Require(v.w==1,"invalid draw/resolve");Require(std::abs(v.x-4)<.00001f,"constant-field noise");z2+=v.y;cap+=v.z;}
  z2/=count;cap/=count;Require(std::abs(z2-.5)<.008&&std::abs(cap-.36)<.008,"cosine integral mismatch");
  std::cout<<"scenario "<<c<<" z2="<<z2<<" cap="<<cap<<'\n';
  for(UINT i=0;i<10;++i){const auto& a=values[count*scenarios+c*20+i*2];const auto& b=values[count*scenarios+c*20+i*2+1];
   const auto native=gi_reuse::GIProducerSample(0,0,897,0,i,10);Require(b.x==native.range,"changed independent range draw");
   if(c==2||c==3||c==5){Require(b.z==0&&a.w==1,"invalid guide not native");Require(std::memcmp(&a,&values[count*scenarios+2*20+i*2],sizeof(V))==0,"fallback directions differ");}
   if(i)Require(std::memcmp(&a,&values[count*scenarios+c*20],3*sizeof(float))!=0,"repeated ray direction");
  }
 }
 D3D12_RANGE empty{};read->Unmap(0,&empty);
 std::cout<<"PASS paired sampler/resolve: six proposal states, distinct rays, range parity, constant fields, cosine integrals. Not native DXR execution.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
