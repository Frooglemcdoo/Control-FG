#include "gpu_test_device.h"
#include "../src/gi_reuse/world_directional_guide.h"
#include <random>
using namespace gi_reuse;
struct Prepared {float rowCDF[8],columnCDF[64];uint generation,frame;};
struct Frozen {GIDirectionalGuide guide;float energy[64];uint generation,frame;};
struct Input {GIPoint normal;float fraction,u,v;uint guideIndex,generation,frame,reserved[3];};
struct Result {GIWorldProposal proposal;uint cell;float contribution[3];uint training;};
struct Learned {GIDirectionalGuide guide;float energy[64];uint parentGeneration,nextFrame,valid,count;};
static_assert(sizeof(Frozen)==520&&sizeof(Input)==48&&sizeof(Result)==48&&sizeof(Learned)==528);
uint Cell(GIPoint d){float u=GIWorldAtan2(d.y,d.x)*.1591549430918953f;if(u<0)u+=1;return std::min(uint(u*8),7u)+8*std::min(uint((d.z+1)*4),7u);}
bool Close(float a,float b){return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=2e-4f+2e-4f*std::abs(b);}
int main()try{
 Device gpu(false);std::vector<Frozen> guides(6);
 for(uint i=0;i<guides.size();++i){auto& g=guides[i];g.frame=100;g.generation=27+i;g.energy[63-i]=2;g.energy[i]=.01f;g.guide=GIBuildDirectionalGuide(g.energy,.5f);}
 GIPoint normals[]={{0,0,1},{0,0,-1},{1,0,0},{0,1,0},{.6f,0,.8f}};
 std::mt19937 gen(424);std::uniform_real_distribution<float> random(.0001f,.9999f);std::vector<Input> in;
 for(uint i=0;i<5;++i)for(uint j=0;j<128;++j)in.push_back({normals[i],j%3==0?0.f:j%3==1?.125f:.5f,random(gen),random(gen),i,guides[i].generation,100,{}});
 for(uint group=0;group<5;++group)for(uint edge=0;edge<8;++edge){auto& v=in[group*128+edge];v.fraction=.5f;v.u=edge%4==0?0.f:edge%4==1?.125f:edge%4==2?.5f:.99999994f;v.v=edge<4?0.f:.99999994f;}
 auto bad=in[0];bad.generation++;in.push_back(bad);bad=in[1];bad.frame--;in.push_back(bad);bad=in[2];bad.guideIndex=100;in.push_back(bad);
 const UINT count=static_cast<UINT>(in.size()),gcount=static_cast<UINT>(guides.size());
 auto old=gpu.Upload(guides),input=gpu.Upload(in);
 std::vector<unsigned char> guard((count+1)*sizeof(Result),0xcd),gGuard((gcount+1)*sizeof(Learned),0xcd);
 auto out=gpu.Buffer(guard.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true),next=gpu.Buffer(gGuard.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
 auto init=gpu.Upload(guard),gInit=gpu.Upload(gGuard);gpu.list->CopyResource(out.Get(),init.Get());gpu.list->CopyResource(next.Get(),gInit.Get());
 gpu.Transition(out.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);gpu.Transition(next.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 D3D12_DESCRIPTOR_RANGE ranges[6]{};D3D12_ROOT_PARAMETER params[6]{};
 for(UINT i=0;i<6;++i){ranges[i].RangeType=i<3?D3D12_DESCRIPTOR_RANGE_TYPE_SRV:D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[i].NumDescriptors=1;ranges[i].BaseShaderRegister=i<3?i:i-3;params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,&ranges[i]};}
 D3D12_ROOT_SIGNATURE_DESC rd{6,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto Make=[&](const char* name){auto code=Read(name);D3D12_COMPUTE_PIPELINE_STATE_DESC d{};d.pRootSignature=root.Get();d.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> p;Check(gpu.device->CreateComputePipelineState(&d,IID_PPV_ARGS(&p)));return p;};
 auto sample=Make("gi-guide-sample.dxil"),learn=Make("gi-guide-learn.dxil"),publish=Make("gi-guide-publish.dxil"),prepare=Make("gi-guide-prepare.dxil");gpu.list->SetComputeRootSignature(root.Get());
 gpu.list->SetComputeRootDescriptorTable(0,gpu.View(old.Get(),0,sizeof(Frozen),gcount,false));gpu.list->SetComputeRootDescriptorTable(1,gpu.View(input.Get(),1,sizeof(Input),count,false));gpu.list->SetComputeRootDescriptorTable(3,gpu.View(out.Get(),2,sizeof(Result),count+1,true));gpu.list->SetComputeRootDescriptorTable(4,gpu.View(next.Get(),3,sizeof(Learned),gcount+1,true));
 auto published=gpu.Buffer(gcount*sizeof(Prepared),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto initialPrepared=gpu.Buffer(gcount*sizeof(Prepared),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 gpu.list->SetComputeRootDescriptorTable(5,gpu.View(initialPrepared.Get(),8,sizeof(Prepared),gcount,true));
 gpu.list->SetPipelineState(prepare.Get());gpu.list->Dispatch(gcount+1,1,1);
 gpu.Transition(initialPrepared.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 gpu.list->SetComputeRootDescriptorTable(2,gpu.View(initialPrepared.Get(),9,sizeof(Prepared),gcount,false));
 gpu.list->SetComputeRootDescriptorTable(5,gpu.View(published.Get(),4,sizeof(Prepared),gcount,true));
 gpu.list->SetPipelineState(sample.Get());gpu.list->Dispatch((count+63)/64+1,1,1);gpu.Uav(out.Get());
 auto before=gpu.Buffer(guard.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);gpu.Transition(out.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.list->CopyResource(before.Get(),out.Get());gpu.Transition(out.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 gpu.list->SetPipelineState(learn.Get());gpu.list->Dispatch(gcount+1,1,1);gpu.Uav(next.Get());
 // Sampling again must still use the original table, despite finished learning.
 gpu.list->SetPipelineState(sample.Get());gpu.list->Dispatch((count+63)/64+1,1,1);
 gpu.Transition(out.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.Transition(next.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 auto read=gpu.Buffer(guard.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST),trained=gpu.Buffer(gGuard.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);gpu.list->CopyResource(read.Get(),out.Get());gpu.list->CopyResource(trained.Get(),next.Get());
 gpu.Transition(next.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 gpu.list->SetPipelineState(publish.Get());gpu.list->Dispatch(gcount+1,1,1);
 gpu.Transition(published.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 auto nextInputs=in;for(auto& v:nextInputs){++v.frame;++v.generation;}auto nextInput=gpu.Upload(nextInputs);
 auto nextOutput=gpu.Buffer(guard.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
 gpu.list->CopyResource(nextOutput.Get(),init.Get());gpu.Transition(nextOutput.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 gpu.list->SetComputeRootDescriptorTable(2,gpu.View(published.Get(),5,sizeof(Prepared),gcount,false));
 gpu.list->SetComputeRootDescriptorTable(1,gpu.View(nextInput.Get(),6,sizeof(Input),count,false));
 gpu.list->SetComputeRootDescriptorTable(3,gpu.View(nextOutput.Get(),7,sizeof(Result),count+1,true));
 gpu.list->SetPipelineState(sample.Get());gpu.list->Dispatch((count+63)/64+1,1,1);
 gpu.Transition(nextOutput.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 auto nextRead=gpu.Buffer(guard.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);gpu.list->CopyResource(nextRead.Get(),nextOutput.Get());
 gpu.Submit();
 Result* results=nullptr;Result* previous=nullptr;Learned* learned=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&results)));Check(before->Map(0,nullptr,reinterpret_cast<void**>(&previous)));Check(trained->Map(0,nullptr,reinterpret_cast<void**>(&learned)));
 Require(!memcmp(results,previous,guard.size()),"Learning changed same-frame sample output");
 float observed[6][64]{};uint counts[6]{};
 for(UINT i=0;i<count;++i){auto v=in[i];auto r=results[i];if(i>=640){Require(!r.proposal.valid&&!r.training,"Stale generation/frame/index admitted");continue;}
  auto cpu=GISampleWorldGuide(guides[v.guideIndex].guide,v.normal,v.fraction,v.u,v.v);Require(cpu.valid==r.proposal.valid&&cpu.trace==r.proposal.trace&&r.training,"GPU validity mismatch");
  Require(Close(cpu.direction.x,r.proposal.direction.x)&&Close(cpu.direction.y,r.proposal.direction.y)&&Close(cpu.direction.z,r.proposal.direction.z)&&Close(cpu.pdf,r.proposal.pdf)&&Close(cpu.throughputWeight,r.proposal.throughputWeight),"GPU proposal/PDF mismatch");Require(Cell(cpu.direction)==r.cell,"World cell differs");
  const float value[3]={r.cell==63?100.f:.2f,r.cell==63?20.f:.3f,r.cell==63?1.f:.4f};for(uint c=0;c<3;++c)Require(Close(value[c]*cpu.throughputWeight,r.contribution[c]),"Contribution mismatch");
  observed[v.guideIndex][r.cell]+=.2126f*r.contribution[0]+.7152f*r.contribution[1]+.0722f*r.contribution[2];++counts[v.guideIndex];
 }
 for(UINT i=0;i<gcount;++i){float energy[64]{};auto r=learned[i];Require(r.count==counts[i]&&r.valid==uint(counts[i]>0)&&r.parentGeneration==guides[i].generation&&r.nextFrame==101,"Learning ledger mismatch");
  for(uint k=0;k<64;++k){energy[k]=counts[i]?guides[i].energy[k]*.8f+.2f*observed[i][k]/counts[i]:0;Require(Close(energy[k],r.energy[k]),"Learning energy mismatch");}auto expected=GIBuildDirectionalGuide(energy,.5f);for(uint k=0;k<64;++k)Require(Close(expected.mass[k],r.guide.mass[k]),"Next guide differs");
 }
 Require(!memcmp(results+count,guard.data()+count*sizeof(Result),sizeof(Result)),"Sample output guard changed");Require(!memcmp(learned+gcount,gGuard.data()+gcount*sizeof(Learned),sizeof(Learned)),"Learning guard changed");
 Result* nextResults=nullptr;Check(nextRead->Map(0,nullptr,reinterpret_cast<void**>(&nextResults)));unsigned changed=0;
 for(UINT i=0;i<count;++i){auto v=nextInputs[i];auto r=nextResults[i];if(i>=640){Require(!r.proposal.valid&&!r.training,"Next frame accepted stale identity");continue;}
  auto cpu=GISampleWorldGuide(learned[v.guideIndex].guide,v.normal,v.fraction,v.u,v.v);
  Require(cpu.valid==r.proposal.valid&&cpu.trace==r.proposal.trace&&r.training,"Next frame validity differs");
  Require(Close(cpu.pdf,r.proposal.pdf)&&Close(cpu.throughputWeight,r.proposal.throughputWeight)&&Close(cpu.direction.x,r.proposal.direction.x)&&Close(cpu.direction.y,r.proposal.direction.y)&&Close(cpu.direction.z,r.proposal.direction.z),"Next frame sampler and PDF not using published guide");
  Require(Cell(cpu.direction)==r.cell,"Next frame world cell differs");const float value[3]={r.cell==63?100.f:.2f,r.cell==63?20.f:.3f,r.cell==63?1.f:.4f};for(uint c=0;c<3;++c)Require(Close(value[c]*cpu.throughputWeight,r.contribution[c]),"Next frame weighted contribution mismatch");
  if(std::abs(r.proposal.pdf-results[i].proposal.pdf)>1e-4)++changed;
 }
 Require(changed>100,"Published guide was not actually used");Require(!memcmp(nextResults+count,guard.data()+count*sizeof(Result),sizeof(Result)),"Next frame guard changed");
 nextRead->Unmap(0,nullptr);read->Unmap(0,nullptr);before->Unmap(0,nullptr);trained->Unmap(0,nullptr);
 std::cout<<"PASS GPU-only publication: 640 next-frame proposals and weights use learned guide, stale identities rejected, no CPU readback between frames, changed PDFs="<<changed<<"\n";
 std::cout<<"PASS WARP: prepared two-level CDF, 640 world-space proposals, rotated normals, CPU/PDF/contribution parity, stale-frame/generation rejection, density-corrected next-guide learning, empty-guide fallback, unchanged same-frame sampling, output guards\n";
 std::cout<<"LIMIT: synthetic fresh shading, standalone compute, no live raygen/evaluator, receiver tracking or RTX performance measurement\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
