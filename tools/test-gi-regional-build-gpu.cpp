#include "gpu_test_device.h"
#include "../src/gi_reuse/regional_target_build.h"
#include <limits>
int main()try{
 using namespace gi_reuse;Device d(false);constexpr UINT capacity=16;
 auto a=GIMakeWorldGuideKey({.1f,.1f,.1f},{0,0,1}),b=a;
 for(float x=2.1f;;x+=.25f){b=GIMakeWorldGuideKey({x,.1f,.1f},{0,0,1});if(GIRegionalSlot(a,capacity,0)==GIRegionalSlot(b,capacity,0))break;}
 const auto record=[](GIWorldGuideKey key,float x,float score){return GIRegionalTraining{key,{{x,0,3},.3f},score,7};};
 std::vector<GIRegionalTraining> records={record(a,1,10),record(a,1.1f,9),record(a,5,2),record(b,30,8),record(b,40,1)};
 auto invalid=record(a,100,1000);invalid.version=6;records.push_back(invalid);
 invalid=record(a,200,1000);invalid.score=std::numeric_limits<float>::quiet_NaN();records.push_back(invalid);
 auto input=d.Upload(records);
 auto cells=d.Buffer(capacity*sizeof(GIRegionalCell),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto output=d.Buffer(capacity*sizeof(GIRegionalTarget),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto read=d.Buffer(2*capacity*sizeof(GIRegionalTarget),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_ROOT_PARAMETER params[4]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[0].Descriptor={0,0};
 params[1].ParameterType=params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor={0,0};params[2].Descriptor={1,0};
 params[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[3].Constants={0,0,4};
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=4;rd.pParameters=params;
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 const char* names[]={"Clear","Claim","First","Second","Freeze"};std::array<ComPtr<ID3D12PipelineState>,5> psos;
 for(UINT i=0;i<5;++i){auto code=Read((std::string("regional-")+names[i]+".dxil").c_str());D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&psos[i])));}
 const auto build=[&](UINT count,UINT frame,UINT destination){
  d.list->SetComputeRootSignature(root.Get());d.list->SetComputeRootShaderResourceView(0,input->GetGPUVirtualAddress());
  d.list->SetComputeRootUnorderedAccessView(1,cells->GetGPUVirtualAddress());d.list->SetComputeRootUnorderedAccessView(2,output->GetGPUVirtualAddress());
  const UINT control[]={count,capacity,frame,7};d.list->SetComputeRoot32BitConstants(3,4,control,0);
  for(UINT i=0;i<5;++i){d.list->SetPipelineState(psos[i].Get());d.list->Dispatch(1,1,1);
   D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;d.list->ResourceBarrier(1,&barrier);}
  d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  d.list->CopyBufferRegion(read.Get(),destination*capacity*sizeof(GIRegionalTarget),output.Get(),0,capacity*sizeof(GIRegionalTarget));
  d.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 };
 build(UINT(records.size()),20,0);build(0,21,1);d.Submit();
 GIRegionalTarget* values=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&values)));
 GIAutoTargets chosen{};
 Require(GIRegionalLookup(values,capacity,a,22,7,4,chosen),"GPU lost first receiver region");
 Require(chosen.validMask==3&&chosen.bounds[0].center.x==1&&chosen.bounds[1].center.x==5,"GPU duplicate or invalid candidate displaced distinct source");
 Require(chosen.counts[0]==3,"invalid training contaminated count");
 Require(GIRegionalLookup(values,capacity,b,22,7,4,chosen),"GPU lost colliding receiver region");
 Require(chosen.validMask==3&&chosen.bounds[0].center.x==30&&chosen.bounds[1].center.x==40,"GPU shared guides across colliding regions");
 Require(chosen.counts[0]==2,"GPU cross-region accumulation");
 for(UINT i=capacity;i<2*capacity;++i)Require(values[i].targets.validMask==0,"empty frame retained stale guide");
 D3D12_RANGE none{};read->Unmap(0,&none);
 std::cout<<"PASS GPU construction -> regional lookup: colliding receivers retain distinct source pairs, covered duplicates/invalid records rejected, empty frame clears publication. No live integration or timing claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
