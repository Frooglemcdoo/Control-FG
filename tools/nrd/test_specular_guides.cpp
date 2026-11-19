#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include "../../src/nrd_specular_guides.h"
#include "nrd_specular_guides_shader.h"
#include "nrd_restore_probe.h"
#include <cstdio>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("D3D12 failure");}
static void Require(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
static unsigned short Half(float x){unsigned bits;std::memcpy(&bits,&x,4);if((bits&0x7fffffff)==0)return static_cast<unsigned short>(bits>>16);return static_cast<unsigned short>(((bits>>16)&0x8000)|((((bits>>23)&255)-112)<<10)|((bits>>13)&1023));}
int main(){try{
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)));ComPtr<ID3D12InfoQueue> messages;Check(device.As(&messages));
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList4> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
 ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");
 constexpr UINT W=64,H=64,L=3;
 auto buffer=[&](UINT64 bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){ComPtr<ID3D12Resource> r;D3D12_HEAP_PROPERTIES hp{};hp.Type=type;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r;};
 std::array<ComPtr<ID3D12Resource>,4> textures;std::vector<ComPtr<ID3D12Resource>> uploads;
 const DXGI_FORMAT formats[]={DXGI_FORMAT_R16_UINT,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM};
 for(UINT i=0;i<4;++i){D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=W;d.Height=H;d.DepthOrArraySize=i<2?L:1;d.MipLevels=1;d.SampleDesc.Count=1;d.Format=formats[i];D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&textures[i])));
  for(UINT layer=0;layer<d.DepthOrArraySize;++layer){D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes;device->GetCopyableFootprints(&d,layer,1,0,&fp,nullptr,nullptr,&bytes);auto upload=buffer(bytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);void* data;Check(upload->Map(0,nullptr,&data));
   for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){auto* p=static_cast<unsigned char*>(data)+fp.Offset+y*fp.Footprint.RowPitch;
    if(i==0){auto* v=reinterpret_cast<unsigned short*>(p);v[x]=x<24?(layer==2?65534:1):x<48?(layer==0?65535:65534):65534;}
    if(i==1){auto* v=reinterpret_cast<unsigned short*>(p)+4*x;v[0]=Half((x+.5f)/W*2-1);v[1]=Half(1-(y+.5f)/H*2);v[2]=x<24&&layer<2?Half(layer==0?2.5f:4.5f):0x7e00;v[3]=0;}
    if(i==2)reinterpret_cast<float*>(p)[x]=.5f;
    if(i==3){p[4*x]=128;p[4*x+1]=128;p[4*x+2]=0;p[4*x+3]=0;}
   }upload->Unmap(0,nullptr);D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=fp;dst.pResource=textures[i].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=layer;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);uploads.push_back(upload);
  }
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={textures[i].Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);
 }
 D3D12_ROOT_PARAMETER params[2]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[0].Constants={0,0,4};params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor={0,0};
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=2;rd.pParameters=params;ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 gi_reuse::ComputeBindingSnapshot native;Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&native.root)));D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=native.root.Get();ps.CS={kNrd_restore_probe,sizeof(kNrd_restore_probe)};Check(device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&native.pipeline)));native.arguments.Root(reinterpret_cast<UINT64>(native.root.Get()));
 gi_reuse::GraphicsTableState graphics;graphics.Reset();
 control_nrd::SpecularGuides guides(device.Get(),W,H,{kNrd_specular_guides,sizeof(kNrd_specular_guides)});
 struct Constants{float rows[12]{};UINT w=W,h=H,layers=L,reserved=0;float clip[16]{};float inverse[2]{1.f/W,1.f/H};float pad[2]{};} c;
 static_assert(sizeof(c)==144);c.rows[0]=c.rows[5]=c.rows[10]=1;c.clip[0]=c.clip[5]=c.clip[10]=c.clip[15]=1;
 std::array<control_nrd::SpecularGuides::External,4> inputs;for(UINT i=0;i<4;++i)inputs[i]={textures[i].Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
 control_nrd::SpecularGuides::Work work;Require(guides.Record(list.Get(),native,graphics,inputs,&c,work),"guide dispatch rejected");
 auto output=guides.Resource(2);auto desc=output->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes;device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);auto readback=buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={output,0,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=output;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
 Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(fence.Get(),1));Check(fence->SetEventOnCompletion(1,event));Require(WaitForSingleObject(event,10000)==WAIT_OBJECT_0,"GPU completion");
 void* data;Check(readback->Map(0,nullptr,&data));for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){float actual=reinterpret_cast<float*>(static_cast<unsigned char*>(data)+footprint.Offset+y*footprint.Footprint.RowPitch)[x];float expected=x<24?2.f/3.05f:x<48?1.f:0.f;Require(std::isfinite(actual)&&std::abs(actual-expected)<.0001f,"hit reduction/miss/end-marker mismatch");}readback->Unmap(0,nullptr);CloseHandle(event);
 for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T size=0;Check(messages->GetMessage(i,nullptr,&size));std::vector<unsigned char> storage(size);auto* msg=reinterpret_cast<D3D12_MESSAGE*>(storage.data());Check(messages->GetMessage(i,msg,&size));if(msg->Severity<=D3D12_MESSAGE_SEVERITY_ERROR){std::fprintf(stderr,"%s\n",msg->pDescription);throw std::runtime_error("D3D12 guide validation error");}}
 std::puts("PASS native specular guide shader: multi-ray minimum, miss, adaptive end marker, stale NaN positions, clean debug layer");return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
