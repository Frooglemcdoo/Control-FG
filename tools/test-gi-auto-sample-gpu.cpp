#include "gpu_test_device.h"
#include "../src/gi_reuse/producer_sampling.h"
int main()try {
 Device d(false);constexpr UINT count=262144,scenarios=24,total=count*scenarios+scenarios*20;
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
 double broadVariance=0;
 for(UINT testCase=0;testCase<scenarios;++testCase){const UINT c=testCase%12,rayCount=testCase>=12?4:10;double constant=0,z2=0,cap=0,moment=0,peak=0;
  for(UINT i=0;i<count;++i){const auto& v=values[testCase*count+i];Require(v.w==1,"invalid draw/resolve");Require(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"nonfinite result");constant+=v.x;z2+=v.y;cap+=v.z;moment+=double(v.y)*v.y;peak=std::max(peak,double(v.y));}
  constant/=count;z2/=count;cap/=count;moment/=count;
  Require(std::abs(constant-4)<.035,"constant expectation mismatch");
  if(c<6)Require(std::abs(z2-.5)<.008&&std::abs(cap-.36)<.008,"cosine integral mismatch");
  if(c==6||c==8){double expected=c==6?1:.8;
   Require(std::abs(z2-expected)<.015,"narrow-source energy mismatch");
   Require(moment-z2*z2<.01&&peak<2,"narrow-source speckle regression");}
  if(c==7)Require(std::abs(z2-1)<.55,"stale guide support/energy mismatch");
  if(c>=10){
   Require(std::abs(z2-1)<.12,"footprint fit changed source mean energy");
   if(c==10)broadVariance=moment-z2*z2;
   else Require(moment-z2*z2<broadVariance*.6,"footprint fit did not reduce narrow-source variance");
  }
  if(c==9)Require(z2==0&&peak==0,"blocked source retained energy");
  std::cout<<"rays "<<rayCount<<" scenario "<<c<<" z2="<<z2<<" cap="<<cap<<" constant="<<constant<<" variance="<<(moment-z2*z2)<<" peak="<<peak<<'\n';
  for(UINT i=0;i<rayCount;++i){const auto& a=values[count*scenarios+testCase*20+i*2];const auto& b=values[count*scenarios+testCase*20+i*2+1];
   const auto native=gi_reuse::GIProducerSample(0,0,897,0,i,rayCount);Require(b.x==native.range,"changed independent range draw");
   if(c==2||c==3||c==5){Require(b.z==0&&a.w==1,"invalid guide not native");Require(std::memcmp(&a,&values[count*scenarios+(testCase>=12?14:2)*20+i*2],sizeof(V))==0,"fallback directions differ");}
   if(i)Require(std::memcmp(&a,&values[count*scenarios+testCase*20],3*sizeof(float))!=0,"repeated ray direction");
  }
 }
 D3D12_RANGE empty{};read->Unmap(0,&empty);
 std::cout<<"PASS frozen invocation seed despite differing native clocks, fresh invocation changes draw; paired sampler/resolve: twenty-four proposal/source states at ten and four rays, fitted versus broad source variance, distinct rays, range parity, mean energy, narrow-source tails and blocked source. Not native DXR execution.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
