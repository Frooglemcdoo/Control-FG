#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_auto_target_binding.h"
#include "../src/gi_reuse/auto_capture_region.h"
int main()try {
 using namespace gi_reuse;Device d(false);
 const auto params=D3D12AutoTargetBinding::RaygenParameters();
 Require(params.size()==4&&D3D12AutoTargetBinding::RecordBytes==96,"regional capture root layout");
 D3D12_ROOT_SIGNATURE_DESC rd{UINT(params.size()),params.data(),0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 const auto code=Read("gi-producer-packet-validation.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 auto guide=d.Upload(std::vector<unsigned char>(GIAutoPublicationBytes));Require(RetainCreatedStorage(guide.Get()),"guide registration");
 std::array<ComPtr<ID3D12Resource>,2> readbacks;
 for(UINT j=0;j<2;++j){
  const UINT rays=j?10:4,count=4,w=600,h=520,x=88,y=8,pixels=512*512;
  const UINT prefix=count*64+pixels*64,bytes=prefix+pixels*(rays+1)*16;
  auto zero=d.Upload(std::vector<unsigned char>(bytes+64));auto output=d.Buffer(bytes+64,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
  d.list->CopyBufferRegion(output.Get(),0,zero.Get(),0,bytes+64);d.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  GIAutoSelection selection{1,1,rays,j?.4f:.5f,77};D3D12AutoTargetBinding binding;Require(binding.Initialize(guide.Get(),selection),"selection");
  const UINT controls[]={w,h,GIAuditStepControl(8,x),GIAuditCapacityControl(count,y,true)};
  std::array<unsigned char,96> record{};std::array<unsigned char,32> identifier{};
  Require(binding.EncodeRaygen(identifier.data(),record,output->GetGPUVirtualAddress(),controls),"record encode");
  UINT64 encoded=0;std::memcpy(&encoded,record.data()+64,8);Require(encoded==output->GetGPUVirtualAddress()&&!std::memcmp(record.data()+72,controls,16),"capture arguments lost or misaligned");
  for(UINT i=60;i<64;++i)Require(!record[i],"descriptor alignment padding");
  d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootShaderResourceView(0,guide->GetGPUVirtualAddress());d.list->SetComputeRoot32BitConstants(1,5,&selection,0);d.list->SetComputeRootUnorderedAccessView(2,output->GetGPUVirtualAddress());d.list->SetComputeRoot32BitConstants(3,4,controls,0);d.list->Dispatch(64,64,1);
  d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  readbacks[j]=d.Buffer(bytes+64,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyBufferRegion(readbacks[j].Get(),0,output.Get(),0,bytes+64);
 }
 d.Submit();
 for(UINT j=0;j<2;++j){const UINT rays=j?10:4,pixels=512*512,prefix=4*64+pixels*64,bytes=prefix+pixels*(rays+1)*16;
  unsigned char* data=nullptr;Check(readbacks[j]->Map(0,nullptr,reinterpret_cast<void**>(&data)));
  for(UINT i=0;i<prefix;++i)Require(data[i]==0,"producer overwrote training or evaluator audit");
  for(UINT i=bytes;i<bytes+64;++i)Require(data[i]==0,"producer packet overflow");
  for(UINT p=0;p<pixels;++p){const auto* v=reinterpret_cast<const float*>(data+prefix+p*(rays+1)*16);UINT stamp=0;std::memcpy(&stamp,v+3,4);Require(v[0]==1&&v[1]==2&&v[2]==3&&stamp==77,"origin or frame corrupted");
   for(UINT r=0;r<rays;++r){const auto* a=v+4*(r+1);Require(std::abs(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]-1)<1e-5&&a[3]==1,"producer sample corrupted");}}
  D3D12_RANGE empty{};readbacks[j]->Unmap(0,&empty);
 }
 std::cout<<"PASS actual sampler packet writes at 4/10 rays, nonzero capture ROI, root-record alignment, preserved training/audit prefix and overflow guard. Compute fixture, not live DXR.\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
