// Execute the production single-layer copy adapter on WARP. Later ray layers
// contain distinct poison data; only layer zero may reach the distance input.
#include "gpu_test_device.h"
#include "../src/rr_reflection_d3d12.h"
using namespace control_rr_reflection;
static bool Current(void* user,Context& out){out=*static_cast<Context*>(user);return true;}
int main(){try{
 for(UINT format:{57u,10u})for(UINT layers:{1u,4u,16u}){
  Device gpu(false);const UINT bytesPerPixel=format==57?2:8;
  Shape shape{64,64,format,layers,1,1,0,0,3};auto desc=D3D12Api::Descriptor(shape);
  auto make=[&](const D3D12_RESOURCE_DESC& d){ComPtr<ID3D12Resource> p;D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;
   Check(gpu.device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&p)));return p;};
  auto source=make(desc);auto destDesc=desc;destDesc.DepthOrArraySize=1;auto destination=make(destDesc);
  std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> fp(layers);UINT64 total=0;
  gpu.device->GetCopyableFootprints(&desc,0,layers,0,fp.data(),nullptr,nullptr,&total);
  auto upload=gpu.Buffer(total,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
  void* mapped=nullptr;D3D12_RANGE none{0,0};Check(upload->Map(0,&none,&mapped));
  for(UINT layer=0;layer<layers;++layer)for(UINT y=0;y<64;++y)for(UINT x=0;x<64*bytesPerPixel;++x)
   static_cast<unsigned char*>(mapped)[fp[layer].Offset+y*fp[layer].Footprint.RowPitch+x]=static_cast<unsigned char>((x*7+y*13+layer*61)%251);
  upload->Unmap(0,nullptr);
  for(UINT layer=0;layer<layers;++layer){D3D12_TEXTURE_COPY_LOCATION from{},to{};
   from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=fp[layer];
   to.pResource=source.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=layer;
   gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
  gpu.Transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
  Context context{1,reinterpret_cast<Address>(gpu.list.Get()),reinterpret_cast<Address>(gpu.queue.Get()),reinterpret_cast<Address>(gpu.device.Get()),1};
  D3D12Api api;std::uint64_t count=0;api.commandCount=&count;api.user=&context;api.current=&Current;
  Require(api.CopyFirstRay(context,reinterpret_cast<Address>(destination.Get()),reinterpret_cast<Address>(source.Get())),"production copy rejected");
  Require(count==1,"copy command counter");
  Require(!api.CopyFirstRay(context,reinterpret_cast<Address>(source.Get()),reinterpret_cast<Address>(source.Get())),"alias accepted");
  auto stale=context;++stale.frame;Require(!api.CopyFirstRay(stale,reinterpret_cast<Address>(destination.Get()),reinterpret_cast<Address>(source.Get())),"stale frame accepted");
  if(layers>1)Require(!api.CopyFirstRay(context,reinterpret_cast<Address>(source.Get()),reinterpret_cast<Address>(destination.Get())),"multilayer destination accepted");
  Require(count==1,"rejection recorded commands");
  gpu.Transition(destination.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT output{};UINT64 readBytes=0;
  gpu.device->GetCopyableFootprints(&destDesc,0,1,0,&output,nullptr,nullptr,&readBytes);
  auto readback=gpu.Buffer(readBytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=destination.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=output;
  gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);gpu.Submit();
  D3D12_RANGE range{0,static_cast<SIZE_T>(readBytes)};Check(readback->Map(0,&range,&mapped));
  for(UINT y=0;y<64;++y)for(UINT x=0;x<64*bytesPerPixel;++x)
   Require(static_cast<unsigned char*>(mapped)[output.Offset+y*output.Footprint.RowPitch+x]==static_cast<unsigned char>((x*7+y*13)%251),"ray-zero bytes differ");
  readback->Unmap(0,&none);
  std::cout<<"PASS production ray-zero copy format="<<format<<" native_layers="<<layers<<" copied_layers=1; later-layer poison, alias/stale/destination rejection\n";
 }
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
