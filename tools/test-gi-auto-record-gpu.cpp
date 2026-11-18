#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_auto_evaluator.h"
#include "../src/gi_reuse/d3d12_auto_timing.h"
int main()try {
 using namespace gi_reuse;Device d(false);
 D3D12_ROOT_PARAMETER p{};p.ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p.Constants={0,0,1};
 D3D12_ROOT_SIGNATURE_DESC rd{1,&p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 const char* noop="[numthreads(1,1,1)] void main() {}";ComPtr<ID3DBlob> nativeCode;
 Check(D3DCompile(noop,strlen(noop),nullptr,nullptr,nullptr,"main","cs_5_1",0,0,&nativeCode,&error));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={nativeCode->GetBufferPointer(),nativeCode->GetBufferSize()};ComPtr<ID3D12PipelineState> nativePSO;
 Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&nativePSO)));
 const char* fixture="cbuffer Selection:register(b0,space19){uint version,sourceFrame,rayCount;float fraction;uint sampleFrame;} RWByteAddressBuffer result:register(u0,space19); [numthreads(8,16,1)]void main(uint3 id:SV_DispatchThreadID){if(all(id==0))result.Store4(0,uint4(rayCount,asuint(fraction),sampleFrame,1234));}";
 ComPtr<ID3DBlob> code;Check(D3DCompile(fixture,strlen(fixture),nullptr,nullptr,nullptr,"main","cs_5_1",0,0,&code,&error));
 D3D12AutoEvaluator evaluator;Check(evaluator.Initialize(d.device.Get(),root.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),{code->GetBufferPointer(),code->GetBufferSize()}));
 ComputeBindingSnapshot bindings;bindings.root=root;bindings.pipeline=nativePSO;bindings.arguments.Root(reinterpret_cast<UINT64>(root.Get()));UINT marker=0;bindings.arguments.Constants(0,0,1,&marker);
 Require(evaluator.Accept(bindings),"fixture native bindings rejected");
 auto guide=d.Upload(std::vector<GIAutoTargets>(1));Require(RetainCreatedStorage(guide.Get()),"guide storage");
 auto training=d.Buffer(4*sizeof(GIAutoTraining),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto read=d.Buffer(32,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 std::array<D3D12AutoTargetBinding,3> targets;std::array<D3D12AutoTiming,2> timing;
 for(UINT i=0;i<3;++i){const UINT rays=i==0?4u:(i==1?10u:8u);Require(targets[i].Initialize(guide.Get(),{1,1,rays,i==0?.5f:.4f,77}),"selection binding");
  if(i==2){Require(!evaluator.Record(d.list.Get(),bindings,targets[i],training.Get(),16,16,8,4),"unsupported count admitted");continue;}
  Require(timing[i].Initialize(d.device.Get()),"timing initialization");
  timing[i].Mark(d.list.Get(),0);timing[i].Mark(d.list.Get(),1);timing[i].Resolve(d.list.Get(),0,2);
  timing[i].Mark(d.list.Get(),2);
  Require(evaluator.Record(d.list.Get(),bindings,targets[i],training.Get(),16,16,8,4),"production evaluator Record rejected supported count");
  timing[i].Mark(d.list.Get(),3);timing[i].Mark(d.list.Get(),4);timing[i].Mark(d.list.Get(),5);timing[i].Resolve(d.list.Get(),2,4);
  d.Transition(training.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  d.list->CopyBufferRegion(read.Get(),16*i,training.Get(),0,16);
  d.Transition(training.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 }
 d.Submit();UINT* values=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&values)));
 for(UINT i=0;i<2;++i)Require(values[4*i]==(i==0?4u:10u)&&values[4*i+2]==77&&values[4*i+3]==1234,"recorded GPU selection/output mismatch");
 D3D12_RANGE empty{};read->Unmap(0,&empty);UINT64 frequency=0;Check(d.queue->GetTimestampFrequency(&frequency));
 for(auto& t:timing){std::array<double,3> ms{};Require(!t.ReadRetired(0,ms),"zero frequency accepted");Require(t.ReadRetired(frequency,ms),"completed split query ranges unreadable");for(auto v:ms)Require(std::isfinite(v)&&v>=0,"invalid GPU duration");}
 std::cout<<"PASS production evaluator Record dispatch/readback at 4 and 10 rays; rejects 8 rays; timestamp ranges resolve and read after completion. Fixture shading, not live native DXR.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
