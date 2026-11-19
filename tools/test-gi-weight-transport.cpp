#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_auto_target_binding.h"
#include "../src/gi_reuse/auto_weight_layout.h"
#include "../src/gi_reuse/auto_pixel_audit.h"
int main()try{
 using namespace gi_reuse;Device d(false);
 const auto base=D3D12AutoTargetBinding::RaygenParameters();std::vector<D3D12_ROOT_PARAMETER> params(base.begin(),base.end());
 D3D12_ROOT_PARAMETER check{};check.ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;check.Descriptor={1,19};params.push_back(check);
 D3D12_ROOT_SIGNATURE_DESC rd{UINT(params.size()),params.data(),0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 ComPtr<ID3D12PipelineState> stages[2];for(UINT i=0;i<2;++i){auto code=Read(i?"weight-read.dxil":"weight-write.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&stages[i])));}
 std::array<ComPtr<ID3D12Resource>,6> reads;
 for(UINT k=0;k<6;++k){const UINT w=37,h=19,rays=k%2?10:4,capacity=12;const bool audit=k>=2;
  const UINT64 prefix=capacity*64+(audit?UINT64(w)*h*64+GIAuditProducerBytes(w,h,rays)+GIAuditRedBytes(w,h):0),weights=GIAutoWeightBytes(w,h,rays),pairBytes=(k>=2&&k<4)?GIAutoPairBytes(w,h):0,bytes=prefix+weights+pairBytes+64;
  auto initial=d.Upload(std::vector<unsigned char>(size_t(bytes),0xA5));auto data=d.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);d.list->CopyBufferRegion(data.Get(),0,initial.Get(),0,bytes);d.Transition(data.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  auto result=d.Buffer(weights,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
  GIAutoSelection selection{1,1,rays,.25f,77};const UINT controls[]={w,h,8u|((k>=2&&k<4)?8388608u:0u),capacity|(audit?65536u:0u)};
  d.list->SetComputeRootSignature(root.Get());d.list->SetComputeRoot32BitConstants(1,5,&selection,0);d.list->SetComputeRootUnorderedAccessView(2,data->GetGPUVirtualAddress());d.list->SetComputeRoot32BitConstants(3,4,controls,0);d.list->SetComputeRootUnorderedAccessView(4,result->GetGPUVirtualAddress());d.list->SetPipelineState(stages[0].Get());d.list->Dispatch((w+7)/8,(h+7)/8,1);
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=data.Get();d.list->ResourceBarrier(1,&b);d.list->SetPipelineState(stages[1].Get());d.list->Dispatch((w+7)/8,(h+7)/8,1);
  d.Transition(result.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.Transition(data.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);reads[k]=d.Buffer(bytes+weights,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyBufferRegion(reads[k].Get(),0,data.Get(),0,bytes);d.list->CopyBufferRegion(reads[k].Get(),bytes,result.Get(),0,weights);
 }
 d.Submit();
 for(UINT k=0;k<6;++k){const UINT w=37,h=19,rays=k%2?10:4;const UINT64 prefix=12*64+(k>=2?UINT64(w)*h*64+GIAuditProducerBytes(w,h,rays)+GIAuditRedBytes(w,h):0),weights=GIAutoWeightBytes(w,h,rays),pairBytes=(k>=2&&k<4)?GIAutoPairBytes(w,h):0,bytes=prefix+weights+pairBytes+64;
  unsigned char* data=nullptr;Check(reads[k]->Map(0,nullptr,reinterpret_cast<void**>(&data)));for(UINT64 i=0;i<prefix;++i)Require(data[i]==0xA5,"prefix overwritten");for(UINT64 i=prefix+weights+pairBytes;i<bytes;++i)Require(data[i]==0xA5,"tail overwritten");
  const auto* produced=reinterpret_cast<const UINT*>(data+prefix);const auto* consumed=reinterpret_cast<const UINT*>(data+bytes);
  for(UINT y=0;y<h;++y)for(UINT x=0;x<w;++x)for(UINT ray=0;ray<rays;++ray){UINT bits=(x+y*w+ray)%4==0?0:0x3e000001u+(x*37u+y*73u+ray*1009u)%0x1000000u;if(x==1&&y==0)bits=ray+1;const UINT index=(y*w+x)*rays+ray;Require(produced[index]==bits&&consumed[index]==bits,"weight not bit-exact");}
  if(pairBytes){const auto* pairs=reinterpret_cast<const UINT*>(data+prefix+weights);
   for(UINT i=0;i<w*h;++i){const UINT* q=pairs+i*8;
    Require(q[0]==produced[i*rays]&&q[1]==0x3f800001u&&q[2]==0&&q[3]==3&&q[4]==0&&q[5]==0x3f000001u&&q[6]==produced[i*rays+rays-1]&&q[7]==3,"paired RGB/flags layout mismatch");
   }
  }
  D3D12_RANGE empty{};reads[k]->Unmap(0,&empty);
 }
 std::cout<<"PASS 4/10-ray producer-to-consumer weights bit-exact, normal/capture layouts, odd extents, zero/subnormal, prefix/tail guards; capture-only full-resolution paired RGB/flags.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
