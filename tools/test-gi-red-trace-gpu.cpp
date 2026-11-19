#include "gpu_test_device.h"
#include "../src/gi_reuse/auto_pixel_audit.h"
#include <array>
int main()try{
 using namespace gi_reuse;for(UINT test=0;test<4;++test){Device d(false);
 D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[0].Descriptor={0,0};p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={0,0,4};
 D3D12_ROOT_SIGNATURE_DESC desc{2,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-red-trace-validation.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 const UINT w=test<2?19:512,h=test<2?17:512,rays=test%2?10:4,capacity=7;
  const UINT64 prefix=capacity*64ull+UINT64(w)*h*64+GIAuditProducerBytes(w,h,rays),bytes=prefix+GIAuditRedBytes(w,h)+32;
  std::vector<UINT> sentinel(size_t(bytes/4),0xcdcdcdcd);auto upload=d.Upload(sentinel);auto output=d.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);auto read=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  d.list->CopyBufferRegion(output.Get(),0,upload.Get(),0,bytes);d.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  const UINT controls[]={w,h,capacity,rays};d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootUnorderedAccessView(0,output->GetGPUVirtualAddress());d.list->SetComputeRoot32BitConstants(1,4,controls,0);d.list->Dispatch((w*h+63)/64,1,1);
  d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyResource(read.Get(),output.Get());d.Submit();
  unsigned char* data=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&data)));Require(std::memcmp(data,sentinel.data(),size_t(prefix))==0,"red trace overwrote training/pixel/producer prefix");Require(std::memcmp(data+bytes-32,sentinel.data(),32)==0,"red trace overflow");
  const auto* records=reinterpret_cast<const GIAutoRedAudit*>(data+prefix);
  for(UINT i=0;i<w*h;++i){const auto& r=records[i];Require(r.raw[0]==2&&r.raw[1]==3&&r.raw[2]==4&&r.weight==1.25f&&r.secondary[0]==5&&r.secondary[2]==7&&r.material==123&&r.position[0]==8&&r.position[2]==10&&r.ray==rays-1&&r.throughput[0]==11&&r.throughput[2]==13&&r.scale==.5f,"red trace CPU/GPU layout mismatch");}
  D3D12_RANGE empty{};read->Unmap(0,&empty);
 }
 std::cout<<"PASS red trace ABI, odd/even ROI, 4/10 rays, disabled write and protected prefix/tail.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
