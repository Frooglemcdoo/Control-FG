// Execute the shipped temporal+spatial shaders with spatial output fed into
// the next frame, matching Control's native feedback ordering. No game hooks.
#include "gpu_test_device.h"
#include <array>
#include "../build/gi_owned_denoise.generated.h"
#include "../build/gi_owned_spatial.generated.h"
constexpr UINT W=32,H=16,N=32;
using Pixel=std::array<float,4>;
using Image=std::vector<Pixel>;
struct Tex{ComPtr<ID3D12Resource> r;D3D12_PLACED_SUBRESOURCE_FOOTPRINT f{};UINT64 bytes=0;};
struct Harness{
 Device gpu{false};ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> temporal,spatial;
 UINT slot=0;
 Harness(bool old){
  gpu.views.Reset();D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2048;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  Check(gpu.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&gpu.views)));ID3D12DescriptorHeap* heaps[]={gpu.views.Get()};gpu.list->SetDescriptorHeaps(1,heaps);
  D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,6,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
  D3D12_ROOT_PARAMETER params[4]{};for(UINT i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;params[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i+2].DescriptorTable={1,&ranges[i]};}
  D3D12_STATIC_SAMPLER_DESC sm{};sm.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;sm.AddressU=sm.AddressV=sm.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sm.MaxLOD=D3D12_FLOAT32_MAX;sm.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sm.MaxAnisotropy=1;sm.ShaderRegister=6;sm.RegisterSpace=1;
  D3D12_ROOT_SIGNATURE_DESC rd{4,params,1,&sm,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob>b,e;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e));Check(gpu.device->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&root)));
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={kGIOwnedDenoise,sizeof(kGIOwnedDenoise)};
  ComPtr<ID3DBlob> oldCode;
  if(old){Check(D3DCompileFromFile(L"tools/fixtures/gi-owned-r368.hlsl",nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&oldCode,&e));pd.CS={oldCode->GetBufferPointer(),oldCode->GetBufferSize()};}
  Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&temporal)));pd.CS={kGIOwnedSpatial,sizeof(kGIOwnedSpatial)};Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&spatial)));
 }
 Tex Texture(const Image* data=nullptr){
  Tex t;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=W;d.Height=H;d.DepthOrArraySize=d.MipLevels=1;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  Check(gpu.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,data?D3D12_RESOURCE_STATE_COPY_DEST:D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&t.r)));gpu.retained.push_back(t.r);
  gpu.device->GetCopyableFootprints(&d,0,1,0,&t.f,nullptr,nullptr,&t.bytes);
  if(data){std::vector<unsigned char> bytes(static_cast<size_t>(t.bytes));for(UINT y=0;y<H;++y)std::memcpy(bytes.data()+t.f.Offset+y*t.f.Footprint.RowPitch,data->data()+y*W,W*sizeof(Pixel));auto u=gpu.Upload(bytes);D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=t.r.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.pResource=u.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=t.f;gpu.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);gpu.Transition(t.r.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);}
  return t;
 }
 D3D12_GPU_DESCRIPTOR_HANDLE Views(const std::array<ID3D12Resource*,6>& inputs,ID3D12Resource* output){
  const UINT first=slot;for(UINT i=0;i<7;++i){auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(slot++)*gpu.descriptorSize;
   if(i==6){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;gpu.device->CreateUnorderedAccessView(output,nullptr,&v,cpu);}
   else{D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;gpu.device->CreateShaderResourceView(inputs[i],&v,cpu);}}
  auto h=gpu.views->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(first)*gpu.descriptorSize;return h;
 }
 void Dispatch(ID3D12PipelineState* pso,const std::array<ID3D12Resource*,6>& inputs,Tex& output,ID3D12Resource* cb0,ID3D12Resource* cb1){
  auto srv=Views(inputs,output.r.Get()),uav=srv;uav.ptr+=UINT64(6)*gpu.descriptorSize;
  gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetComputeRootConstantBufferView(0,cb0->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(1,cb1->GetGPUVirtualAddress());gpu.list->SetComputeRootDescriptorTable(2,srv);gpu.list->SetComputeRootDescriptorTable(3,uav);gpu.list->SetPipelineState(pso);gpu.list->Dispatch(W/8,H/8,1);gpu.Transition(output.r.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 }
};
struct Stats{double error=0,delta=0,mean=0;float maxValue=0,lightOff=0;};
static Stats Run(bool old,unsigned mode){
 Harness h(old);auto& g=h.gpu;
 Image flat(W*H,Pixel{.5f,.5f,.5f,.5f}),mv(W*H,Pixel{mode==1?1.f/W:0.f,0,0,0});
 auto depth=h.Texture(&flat),g1=h.Texture(&flat),g2=h.Texture(&flat),motion=h.Texture(&mv);
 Image initial(W*H,Pixel{.2f,.3f,.1f,.2f});auto history=h.Texture(&initial);
 std::array<float,192> sys{};sys[0]=sys[4]=float(W);sys[1]=sys[5]=float(H);sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;
 auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
 std::array<UINT,64> filter{};auto cb1=g.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
 std::vector<Tex> frames;std::vector<ComPtr<ID3D12Resource>> reads;
 for(UINT f=0;f<N;++f){
  Image raw(W*H);
  for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){
   UINT id=y*W+x;UINT bits=(id*1664525u+f*1013904223u+17u);bits^=bits>>16;bits*=2246822519u;bits^=bits>>13;
   float noise=float(bits&65535)/65535.f-.5f;
   const bool off=mode==2&&f>=24;
   raw[id]=off?Pixel{.02f,.03f,.01f,.02f}:Pixel{.2f+noise*.3f,.3f+noise*.4f,.1f+noise*.15f,0};
   // Two adjacent green fireflies, a different location every frame.
   const UINT peak=4+(f*7)%24;
   if(mode!=3&&!off&&y==8&&(x==peak||x==peak+1))raw[id]=Pixel{4,30,2,4};
  }
  auto current=h.Texture(&raw),temporal=h.Texture(),spatial=h.Texture();
  h.Dispatch(h.temporal.Get(),{depth.r.Get(),depth.r.Get(),motion.r.Get(),current.r.Get(),history.r.Get(),current.r.Get()},temporal,cb0.Get(),cb1.Get());
  h.Dispatch(h.spatial.Get(),{g1.r.Get(),g2.r.Get(),depth.r.Get(),depth.r.Get(),motion.r.Get(),temporal.r.Get()},spatial,cb0.Get(),cb1.Get());
  g.Transition(spatial.r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
  auto read=g.Buffer(spatial.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=spatial.f;src.pResource=spatial.r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  g.Transition(spatial.r.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  history=spatial;frames.push_back(spatial);reads.push_back(read);
 }
 g.Submit();Stats s;Image previous(W*H);double count=0;
 for(UINT f=0;f<N;++f){void* data=nullptr;Check(reads[f]->Map(0,nullptr,&data));auto& t=frames[f];
  for(UINT y=2;y<H-2;++y)for(UINT x=4;x<W-4;++x){auto* p=reinterpret_cast<const float*>(static_cast<const unsigned char*>(data)+t.f.Offset+y*t.f.Footprint.RowPitch)+x*4;
   Require(std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]),"nonfinite sequence output");Require(std::abs(p[0]-p[3])<1e-5f,"red/alpha ABI");
   if(mode==2&&f>=24)s.lightOff=std::max(s.lightOff,std::abs(p[1]-.03f));
   if(f>=4 && (mode!=2||f<24)){s.error+=(p[1]-.3)*(p[1]-.3);s.delta+=(p[1]-previous[y*W+x][1])*(p[1]-previous[y*W+x][1]);s.mean+=p[1];s.maxValue=std::max(s.maxValue,p[1]);++count;}
   previous[y*W+x]={p[0],p[1],p[2],p[3]};
  }reads[f]->Unmap(0,nullptr);
 }
 s.error=std::sqrt(s.error/count);s.delta=std::sqrt(s.delta/count);s.mean/=count;return s;
}
int main(){try{
 for(unsigned mode=0;mode<4;++mode){auto before=Run(true,mode),after=Run(false,mode);
  std::cout<<"R369 sequence mode="<<mode<<" old_rmse="<<before.error<<" new_rmse="<<after.error<<" old_delta="<<before.delta<<" new_delta="<<after.delta<<" mean="<<after.mean<<" peak="<<after.maxValue<<" light_off_error="<<after.lightOff<<'\n';
  Require(std::abs(after.mean-.3)<.025,"diffuse mean brightness changed");
  if(mode!=3){Require(after.error<before.error*.35,"firefly RMSE reduction insufficient");Require(after.delta<before.delta*.5,"boiling reduction insufficient");Require(after.maxValue<.5,"persistent firefly survived feedback");}
  else Require(after.error<.035,"unbiased diffuse noise not converging");
  if(mode==2)Require(after.lightOff<.0001f,"destroyed light persisted in history");
 }
 std::cout<<"PASS R369 32-frame production temporal/spatial feedback, moving reprojection, colored adjacent fireflies, unbiased energy, destroyed light and alpha ABI\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
