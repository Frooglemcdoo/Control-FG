#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include <array>
#include "pt-glass-native-root-fixture.h"
#include "../src/gi_excess_redistribution_d3d12.h"
#include "../build/gi_excess.generated.h"
constexpr UINT W=9,H=9;
using Pixel=std::array<float,4>;
static void Run(unsigned mode){
 Device g(false);ComPtr<ID3D12InfoQueue> messages;Check(g.device.As(&messages));
 ComPtr<ID3D12RootSignature> root;Check(CreateGlassNativeRootFixture(g.device.Get(),&root));
 gi_excess::Pipeline pipeline(g.device.Get(),root.Get(),{kGIExcessClassify,sizeof(kGIExcessClassify)},{kGIExcessGather,sizeof(kGIExcessGather)});
 D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=root.Get();ps.CS={kGIExcessClassify,sizeof(kGIExcessClassify)};
 ComPtr<ID3D12PipelineState> native;Check(g.device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&native)));
 std::array<std::vector<Pixel>,4> data;
 data[0].assign(W*H,Pixel{.5f,.5f,.5f,0});data[1].assign(W*H,Pixel{0,0,0,1.f/255});
 data[2].assign(W*H,Pixel{1,0,0,0});data[3].assign(W*H,Pixel{.01f,.01f,.01f,1});
 std::vector<std::array<UINT,4>> coverage(W*H,{0xffffu,0,0,0});
 const UINT px=mode==0?0:3,py=mode==0?0:4;
 if(mode==6)for(auto& p:data[3])p={2,1,.5f,1};
 else data[3][py*W+px]={2,1,.5f,1};
 for(UINT y=0;y<H;++y)for(UINT x=4;x<W;++x){const UINT i=y*W+x;
  if(mode==1)data[2][i][0]=2;
  if(mode==2)data[1][i][3]=2.f/255;
  if(mode==3)data[0][i][0]=.9f;
  if(mode==4)data[0][i][3]=1.f/255;
  if(mode==5)coverage[i][0]=0xfffeu;
 }
 if(mode==7)coverage[py*W+px][0]=0xfffeu;
 std::array<ComPtr<ID3D12Resource>,5> textures;
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 size=0;
 for(UINT i=0;i<5;++i){
  D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=W;d.Height=H;d.DepthOrArraySize=d.MipLevels=1;
  d.Format=i==4?DXGI_FORMAT_R32G32B32A32_UINT:DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  Check(g.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&textures[i])));
  g.device->GetCopyableFootprints(&d,0,1,0,&footprint,nullptr,nullptr,&size);
  std::vector<unsigned char> bytes(static_cast<size_t>(size));
  for(UINT y=0;y<H;++y)std::memcpy(bytes.data()+footprint.Offset+y*footprint.Footprint.RowPitch,
   i==4?static_cast<const void*>(coverage.data()+y*W):static_cast<const void*>(data[i].data()+y*W),W*16);
  auto upload=g.Upload(bytes);D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=textures[i].Get();src.pResource=upload.Get();
  src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprint;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  g.Transition(textures[i].Get(),D3D12_RESOURCE_STATE_COPY_DEST,i==3?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 }
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=PTGlass::LightingTableLayout::NativeMaterialPrefix+98;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
 ComPtr<ID3D12DescriptorHeap> visible,cpu;Check(g.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&visible)));
 hd.NumDescriptors=49;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;Check(g.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu)));
 auto handle=[&](ID3D12DescriptorHeap* heap,UINT i){auto h=heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*g.descriptorSize;return h;};
 D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Texture2D.MipLevels=1;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=srv.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
 for(UINT i=0;i<5;++i)g.device->CreateConstantBufferView(nullptr,handle(cpu.Get(),i));
 for(UINT i=5;i<41;++i)g.device->CreateShaderResourceView(nullptr,&srv,handle(cpu.Get(),i));
 for(UINT i=41;i<49;++i)g.device->CreateUnorderedAccessView(nullptr,nullptr,&uav,handle(cpu.Get(),i));
 for(UINT i=0;i<3;++i)g.device->CreateShaderResourceView(textures[i].Get(),&srv,handle(cpu.Get(),i+5));
 srv.Format=DXGI_FORMAT_R32G32B32A32_UINT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;srv.Texture2DArray.MipLevels=1;srv.Texture2DArray.ArraySize=1;
 g.device->CreateShaderResourceView(textures[4].Get(),&srv,handle(cpu.Get(),29));
 g.device->CreateUnorderedAccessView(textures[3].Get(),nullptr,&uav,handle(cpu.Get(),41));
 const UINT first=PTGlass::LightingTableLayout::NativeMaterialPrefix;
 g.device->CopyDescriptorsSimple(49,handle(visible.Get(),first),handle(cpu.Get(),0),hd.Type);
 PTGlass::LightingTableShadow shadow;Check(shadow.Initialize(g.device.Get()));Require(shadow.Begin(1,{visible.Get(),first,49}),"shadow begin");
 Require(shadow.Copy(1,0,{cpu.Get(),0,49}),"shadow bindings");
 auto table=visible->GetGPUDescriptorHandleForHeapStart();table.ptr+=UINT64(first)*g.descriptorSize;
 ID3D12DescriptorHeap* heaps[]={visible.Get()};g.list->SetDescriptorHeaps(1,heaps);g.list->SetComputeRootSignature(root.Get());g.list->SetComputeRootDescriptorTable(0,table);g.list->SetPipelineState(native.Get());
 gi_excess::Pipeline::Work work;
 Require(pipeline.Record(g.list.Get(),native.Get(),table,textures[3].Get(),shadow,1,{visible.Get(),first+49,49},work),"production Record");
 g.Transition(textures[3].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 auto read=g.Buffer(size,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;src.pResource=textures[3].Get();
 g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);g.Submit();
 void* mapped=nullptr;Check(read->Map(0,nullptr,&mapped));
 double before[3]{},after[3]{};float peak=0;
 for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){
  auto p=reinterpret_cast<const float*>(static_cast<const unsigned char*>(mapped)+footprint.Offset+y*footprint.Footprint.RowPitch)+4*x;
  for(UINT c=0;c<3;++c){Require(std::isfinite(p[c])&&p[c]>=0,"invalid output");before[c]+=data[3][y*W+x][c];after[c]+=p[c];}
  peak=std::max(peak,p[0]);Require(p[3]==1,"raw alpha changed");
  if((mode>=1&&mode<=5&&x>=4)||mode>=6)for(UINT c=0;c<3;++c)Require(std::abs(p[c]-data[3][y*W+x][c])<1e-6f,"boundary/unchanged pixel");
 }
 read->Unmap(0,nullptr);
 for(UINT c=0;c<3;++c)Require(std::abs(after[c]-before[c])<2e-5,"per-channel energy loss");
 if(mode<6)Require(peak<.3f,"isolated peak not reduced");
 for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T n=0;Check(messages->GetMessage(i,nullptr,&n));std::vector<char> bytes(n);auto m=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(messages->GetMessage(i,m,&n));Require(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR,m->pDescription);}
 std::cout<<"PASS excess GPU case "<<mode<<" energy and surface boundaries\n";
}
int main(){try{
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 for(unsigned mode=0;mode<8;++mode)Run(mode);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
