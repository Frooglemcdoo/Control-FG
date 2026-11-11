#include "gpu_test_device.h"
int main() try {
 for(UINT scenario=0;scenario<3;++scenario){
  Device d(false);constexpr UINT capacity=6,words=(16+capacity*256+256)/4;
  std::vector<UINT> init(words,0xa5a5a5a5);init[0]=0;init[1]=0x4f474931;init[2]=256;init[3]=123;
  auto upload=d.Upload(init);auto out=d.Buffer(words*4,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
  auto back=d.Buffer(words*4,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[0].Descriptor={0,2};
  p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={0,2,8};
  D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=2;rd.pParameters=p;
  ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
  ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
  auto code=Read("gi-owned-source-recorder.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
  ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
  d.list->CopyBufferRegion(out.Get(),0,upload.Get(),0,words*4);d.Transition(out.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  UINT constants[8]={2,3,scenario==1?3u:2u,1,capacity,123,scenario==2?0u:1u,0};
  d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootUnorderedAccessView(0,out->GetGPUVirtualAddress());
  d.list->SetComputeRoot32BitConstants(1,8,constants,0);d.list->Dispatch(1,1,1);
  d.Transition(out.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(back.Get(),0,out.Get(),0,words*4);d.Submit();
  UINT* data=nullptr;D3D12_RANGE range{0,words*4};Check(back->Map(0,&range,reinterpret_cast<void**>(&data)));
  Require(data[0]==(scenario==2?0u:scenario==1?9u:6u),"Reservation count mismatch");
  for(UINT i=4+capacity*64;i<words;++i)Require(data[i]==0xa5a5a5a5,"Recorder overwrote guard");
  if(scenario==2){for(UINT i=4;i<words;++i)Require(data[i]==0xa5a5a5a5,"Disabled trace touched records");}
  else for(UINT i=0;i<capacity;++i){const UINT* r=data+4+i*64;Require(r[51]==123,"Frame token mismatch");Require(r[0]>=2&&r[0]<constants[0]+constants[2]&&r[1]==3,"ROI mismatch");for(UINT j=60;j<64;++j)Require(r[j]==0,"Reserved words not cleared");}
  if(scenario==0){std::ofstream f("build/owned-source-recorder-fixture.bin",std::ios::binary);f.write(reinterpret_cast<char*>(data),(16+capacity*256));Require(bool(f),"Fixture export failed");}
  D3D12_RANGE none{0,0};back->Unmap(0,&none);
 }
 std::cout<<"OGI1 GPU recorder passed: ROI, complete ray sets, overflow guard, disabled stores.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
