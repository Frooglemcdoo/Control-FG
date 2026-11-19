#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_auto_target_binding.h"
#include "../src/gi_reuse/auto_pixel_audit.h"
#include "../src/gi_reuse/auto_weight_layout.h"
#include "../src/gi_reuse/motion_capture_layout.h"
int main()try{
 using namespace gi_reuse;Device d(false);const auto parameters=D3D12AutoTargetBinding::RaygenParameters();
 D3D12_ROOT_SIGNATURE_DESC rd{UINT(parameters.size()),parameters.data(),0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 ComPtr<ID3D12PipelineState> psos[2];for(unsigned i=0;i<2;++i){auto code=Read(i?"motion-evaluator.dxil":"motion-producer.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&psos[i])));}
 auto guide=d.Upload(std::vector<unsigned char>(GIAutoPublicationBytes));std::array<ComPtr<ID3D12Resource>,4> reads;std::array<UINT64,4> offsets{},sizes{};
 for(unsigned test=0;test<4;++test){const UINT w=64,h=40,rays=test%2?10:4,capacity=40;const bool enabled=test<2;
  const UINT64 base=capacity*64+UINT64(w)*h*64+GIAuditProducerBytes(w,h,rays)+GIAuditRedBytes(w,h)+GIAutoWeightBytes(w,h,rays);offsets[test]=base;sizes[test]=base+gi_motion::Bytes(w,h,rays);
  auto zero=d.Upload(std::vector<unsigned char>(size_t(sizes[test]+64)));auto buffer=d.Buffer(sizes[test]+64,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);d.list->CopyBufferRegion(buffer.Get(),0,zero.Get(),0,sizes[test]+64);d.Transition(buffer.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  const GIAutoSelection selection{1,1,rays,.25f,77};const UINT controls[]={w,h,8u|(enabled?gi_motion::ControlBit:0u),capacity|65536u};
  d.list->SetComputeRootSignature(root.Get());d.list->SetComputeRootShaderResourceView(0,guide->GetGPUVirtualAddress());d.list->SetComputeRoot32BitConstants(1,5,&selection,0);d.list->SetComputeRootUnorderedAccessView(2,buffer->GetGPUVirtualAddress());d.list->SetComputeRoot32BitConstants(3,4,controls,0);
  for(unsigned pass=0;pass<2;++pass){d.list->SetPipelineState(psos[pass].Get());d.list->Dispatch(8,5,1);D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=buffer.Get();d.list->ResourceBarrier(1,&b);}
  d.Transition(buffer.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);reads[test]=d.Buffer(sizes[test]+64,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyBufferRegion(reads[test].Get(),0,buffer.Get(),0,sizes[test]+64);
 }
 d.Submit();
 for(unsigned test=0;test<4;++test){unsigned char* bytes=nullptr;Check(reads[test]->Map(0,nullptr,reinterpret_cast<void**>(&bytes)));const auto base=offsets[test],end=sizes[test];
  for(UINT64 i=0;i<(test<2?base:end+64);++i)Require(bytes[i]==0,"motion capture altered existing training/weight lanes or disabled capture wrote data");
  for(UINT64 i=end;i<end+64;++i)Require(bytes[i]==0,"motion tail overflow");
  if(test<2){const auto* header=reinterpret_cast<const UINT*>(bytes+base);Require(header[284]==0x4d343834u&&header[285]==77&&header[286]==64&&header[287]==40,"motion header mismatch");
   const UINT rays=test%2?10:4;
   for(unsigned pixel=0;pixel<64*40;++pixel)for(unsigned ray=0;ray<rays;++ray){const auto* u=reinterpret_cast<const UINT*>(bytes+base+2048+(UINT64(pixel)*rays+ray)*224);const auto* f=reinterpret_cast<const float*>(u);
    Require(u[3]==77&&f[0]==float(pixel%64)&&f[1]==float(pixel/64)&&f[2]==3&&f[6]==1&&f[7]==1,"actual producer record corrupt");
    if(!pixel){for(unsigned k=8;k<56;++k)Require(u[k]==0,"invalid receiver shade records not cleared");continue;}
    Require(f[8]==.125f&&f[9]==.0625f&&f[10]==.03125f&&f[11]==1&&u[15]==1885&&u[19]==(ray|0x80000000u),"per-ray evaluator record corrupt");
    for(unsigned t=0;t<8;++t)for(unsigned k=0;k<4;++k)Require(f[24+t*4+k]==float(t+1),"lighting term lost precision or wrong offset");
   }
  }
  D3D12_RANGE empty{};reads[test]->Unmap(0,&empty);
 }
 std::cout<<"PASS full-screen 4/10-ray producer/evaluator ABI, invalid receiver, disabled capture, preserved prefix and overflow guards on WARP\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
