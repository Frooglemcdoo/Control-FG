#include <windows.h>
#include <dxgi1_6.h>
#include "../../src/nrd_diffuse_d3d12.h"
#include <cstdio>
#include <array>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("GPU test failure");}
static void Require(bool good,const char*s){if(!good)throw std::runtime_error(s);}
constexpr UINT W=64,H=32;
int main(){try{
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)));
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
 ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");
 auto buffer=[&](UINT64 size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){ComPtr<ID3D12Resource> p;D3D12_HEAP_PROPERTIES hp{};hp.Type=type;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&p)));return p;};
 std::array<ComPtr<ID3D12Resource>,5> textures;
 D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=W;td.Height=H;td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
 for(auto&t:textures)Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&t)));
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;device->GetCopyableFootprints(&td,0,1,0,&footprint,nullptr,nullptr,&bytes);
 auto readback=buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 control_nrd::Session session;control_nrd::D3D12Backend backend(device.Get(),session.Description(),W,H);
 control_nrd::D3D12Backend::Inputs inputs;
 const nrd::ResourceType names[]={nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,nrd::ResourceType::IN_NORMAL_ROUGHNESS,nrd::ResourceType::IN_VIEWZ,nrd::ResourceType::IN_MV,nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST};
 for(UINT i=0;i<5;++i)inputs[names[i]]={textures[i].Get(),D3D12_RESOURCE_STATE_COMMON};
 control_nrd::Frame f;f.view=1;f.width=W;f.height=H;f.sameFrameGuides=f.worldNormals=f.linearViewDepth=f.unjitteredMotion=f.radianceContract=f.hitDistanceContract=true;
 auto&s=f.settings;s.timeDeltaBetweenFrames=16.6667f;for(UINT i=0;i<16;++i)s.worldToViewMatrix[i]=s.worldToViewMatrixPrev[i]=i%5==0?1.f:0.f;
 s.viewToClipMatrix[0]=s.viewToClipMatrix[5]=1;s.viewToClipMatrix[10]=1.001f;s.viewToClipMatrix[11]=1;s.viewToClipMatrix[14]=-.1001f;std::memcpy(s.viewToClipMatrixPrev,s.viewToClipMatrix,64);
 auto transition=[&](control_nrd::D3D12Backend::External&t,D3D12_RESOURCE_STATES next){if(t.state==next)return;D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={t.resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,t.state,next};list->ResourceBarrier(1,&b);t.state=next;};
 double finalMean=0;
 for(UINT frame=1;frame<=16;++frame){
  Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));
  std::vector<ComPtr<ID3D12Resource>> uploads;
  for(UINT i=0;i<4;++i){auto upload=buffer(bytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);void*p=nullptr;Check(upload->Map(0,nullptr,&p));
   for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){auto*pixel=reinterpret_cast<float*>(static_cast<unsigned char*>(p)+footprint.Offset+y*footprint.Footprint.RowPitch)+x*4;
    std::array<float,4> value=i==0?std::array<float,4>{.4f,.4f,.4f,2.f}:i==1?std::array<float,4>{0,0,-1,1}:i==2?std::array<float,4>{10,0,0,0}:std::array<float,4>{0,0,0,0};std::memcpy(pixel,value.data(),16);
   }upload->Unmap(0,nullptr);transition(inputs[names[i]],D3D12_RESOURCE_STATE_COPY_DEST);
   D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=textures[i].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprint;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);uploads.push_back(upload);
  }
  f.frame=frame;Require(session.Prepare(f,1),"plan rejected");control_nrd::D3D12Backend::Recording recording;backend.Record(list.Get(),session.Plan(),inputs,recording);
  transition(inputs[names[4]],D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;src.pResource=textures[4].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(fence.Get(),frame));Require(session.Submitted(1,frame),"submission rejected");
  Check(fence->SetEventOnCompletion(frame,event));Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"GPU timeout");
  void*p=nullptr;Check(readback->Map(0,nullptr,&p));double sum=0;for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){const auto*pixel=reinterpret_cast<const float*>(static_cast<unsigned char*>(p)+footprint.Offset+y*footprint.Footprint.RowPitch)+x*4;Require(std::isfinite(pixel[0]),"nonfinite RELAX output");sum+=pixel[0];}readback->Unmap(0,nullptr);finalMean=sum/(W*H);
  // Remove all recorded references before releasing this frame's descriptors.
  Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));Check(list->Close());
 }
 CloseHandle(event);std::printf("RELAX uniform mean %.8f\n",finalMean);Require(std::abs(finalMean-.4)<.015,"RELAX lost uniform light");std::puts("PASS actual RELAX_DIFFUSE D3D12 dispatch chain and independent history");return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
