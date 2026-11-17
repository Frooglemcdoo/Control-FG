#include "gpu_test_device.h"
#include "../src/gi_reuse/auto_target_contract.h"
int main()try {
 using namespace gi_reuse;Device d(false);constexpr UINT count=9;
 std::vector<unsigned char> sentinel((count+1)*sizeof(GIAutoTraining),0xcd);
 auto initial=d.Upload(sentinel);auto training=d.Buffer(sentinel.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
 auto read=d.Buffer(sentinel.size()*2,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.list->CopyResource(training.Get(),initial.Get());d.Transition(training.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 D3D12_ROOT_PARAMETER p[3]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[0].Descriptor={0,19};
 p[1].ParameterType=p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={1,19,4};p[2].Constants={0,0,2};
 D3D12_ROOT_SIGNATURE_DESC rd{3,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-auto-training.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootUnorderedAccessView(0,training->GetGPUVirtualAddress());
 const UINT control[]={19,17,8,count};d.list->SetComputeRoot32BitConstants(1,4,control,0);
 for(UINT phase=0;phase<2;++phase){const UINT test[]={phase,7};d.list->SetComputeRoot32BitConstants(2,2,test,0);d.list->Dispatch(3,3,1);
  d.Transition(training.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(read.Get(),phase*sentinel.size(),training.Get(),0,sentinel.size());
  d.Transition(training.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
 d.Submit();GIAutoTraining* values=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&values)));
 for(UINT phase=0;phase<2;++phase){const auto* row=values+phase*(count+1);
  for(UINT i=0;i<count;++i){const bool valid=phase==0&&i%3==0;Require(row[i].valid==UINT(valid),"stale/invalid training entry");
   if(valid){Require(row[i].position.x==float(i)&&row[i].version==7,"training identity");Require(std::abs(row[i].energy-(2*.2126f+4*.7152f+8*.0722f)*8.f)<.0001f,"bright ray lost or downweighted out of proposal");}}
  Require(std::memcmp(row+count,sentinel.data()+count*sizeof(GIAutoTraining),sizeof(GIAutoTraining))==0,"padded threads overwrite training guard");}
 D3D12_RANGE empty{};read->Unmap(0,&empty);
 std::cout<<"PASS irregular tile coverage, bounded writes, invalid receiver/miss clearing and all-ray maximum and stable raw-brightness proposal score.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
