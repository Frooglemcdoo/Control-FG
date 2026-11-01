#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include "../src/rr_input_capture_limits.h"
using Microsoft::WRL::ComPtr;
struct RRInputCaptureJob {
 unsigned count=0;
 ID3D12Resource* sources[kRRInputCaptureMaxResources]{},*readback[kRRInputCaptureMaxResources]{};
 D3D12_RESOURCE_STATES states[kRRInputCaptureMaxResources]{};
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[kRRInputCaptureMaxResources]{};
 unsigned copySubresource[kRRInputCaptureMaxResources]{};
 D3D12_BOX copyBox[kRRInputCaptureMaxResources]{};bool copyHasBox=false;
};
struct RRGuideInputSnapshot {ID3D12GraphicsCommandList* commandList=nullptr;void* commandContext=nullptr;};
#define RR_SEH_TRY __try
#define RR_SEH_EXCEPT(x) __except(x)
#include "../src/rr_input_capture_record.h"
static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned>(hr)));}
static ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,UINT64 size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
 D3D12_HEAP_PROPERTIES heap{};heap.Type=type;heap.CreationNodeMask=heap.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 ComPtr<ID3D12Resource> r;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r;
}
static void Run(DXGI_FORMAT format,unsigned pixelBytes){
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC q{};Check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
 D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=32;d.Height=24;d.DepthOrArraySize=3;d.MipLevels=1;d.SampleDesc.Count=1;d.Format=format;
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
 ComPtr<ID3D12Resource> source;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&source)));
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[3]{};UINT64 size=0;device->GetCopyableFootprints(&d,0,3,0,fp,nullptr,nullptr,&size);
 auto upload=Buffer(device.Get(),size,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
 void* p=nullptr;D3D12_RANGE empty{0,0};Check(upload->Map(0,&empty,&p));std::memset(p,0,static_cast<size_t>(size));
 for(unsigned slice=0;slice<3;++slice)for(unsigned y=0;y<24;++y)for(unsigned x=0;x<32;++x){
  auto* pixel=static_cast<unsigned char*>(p)+fp[slice].Offset+y*fp[slice].Footprint.RowPitch+x*pixelBytes;
  const unsigned value=slice*10000+y*100+x;
  if(pixelBytes==2){const auto small=static_cast<std::uint16_t>(value);std::memcpy(pixel,&small,2);}
  else {unsigned pair[2]{value,~value};std::memcpy(pixel,pair,8);}
 }upload->Unmap(0,nullptr);
 for(unsigned slice=0;slice<3;++slice){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=fp[slice];to.pResource=source.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=slice;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={source.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&barrier);
 // Six crops with distinct origins, two per slice. Validate the production copy helper.
 auto readback=Buffer(device.Get(),6*2048,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 unsigned long long context[2]{};RRGuideInputSnapshot native{list.Get(),context};
 for(unsigned slice=0;slice<3;++slice)for(unsigned roi=0;roi<2;++roi){RRInputCaptureJob c{};c.count=1;c.sources[0]=source.Get();c.readback[0]=readback.Get();c.states[0]=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;c.copyHasBox=true;c.copySubresource[0]=slice;
  c.copyBox[0]={roi*12+2,3,0,roi*12+10,11,1};c.footprint[0].Offset=(slice*2+roi)*2048;c.footprint[0].Footprint={format,8,8,1,256};DWORD fault=0;bool started=false;
  if(!RRInputCaptureRecord(&c,&native,&fault,&started)||!started||fault)throw std::runtime_error("record failed");
 }
 Check(list->Close());ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
 ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Check(queue->Signal(fence.Get(),1));
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("event");Check(fence->SetEventOnCompletion(1,event));const auto wait=WaitForSingleObject(event,10000);CloseHandle(event);if(wait!=WAIT_OBJECT_0)throw std::runtime_error("fence timeout");
 D3D12_RANGE range{0,6*2048};Check(readback->Map(0,&range,&p));
 for(unsigned slice=0;slice<3;++slice)for(unsigned roi=0;roi<2;++roi)for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){
  const auto* pixel=static_cast<const unsigned char*>(p)+(slice*2+roi)*2048+y*256+x*pixelBytes;
  const unsigned expected=slice*10000+(y+3)*100+x+roi*12+2;
  if(pixelBytes==2){std::uint16_t value=0;std::memcpy(&value,pixel,2);if(value!=expected)throw std::runtime_error("R16 slice/ROI mismatch");}
  else {unsigned pair[2]{};std::memcpy(pair,pixel,8);if(pair[0]!=expected||pair[1]!=~expected)throw std::runtime_error("slice/ROI content mismatch");}
 }readback->Unmap(0,&empty);
}
int main()try{
 Run(DXGI_FORMAT_R16_UINT,2);Run(DXGI_FORMAT_R32G32_UINT,8);Run(DXGI_FORMAT_R16G16B16A16_FLOAT,8);
 std::cout<<"PASS GPU completed R16_UINT, R32G32_UINT and RGBA16_FLOAT, all three slices and both ROI origins with exact pixel values\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
