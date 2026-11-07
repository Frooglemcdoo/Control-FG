// Actual R375 temporal + horizontal/vertical spatial + spatial-result feedback.
#define main OwnedSequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
#include "../build/gi_motion_history.generated.h"
#include "../build/gi_spatial_history.generated.h"
#include "fixtures/gi-spatial-r375.h"
struct Chain:Harness{
 ComPtr<ID3D12PipelineState> passes[2];
 Chain(bool old):Harness(false){
  D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,8,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
  D3D12_ROOT_PARAMETER ps[4]{};for(UINT i=0;i<2;++i){ps[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;ps[i].Descriptor.ShaderRegister=i;ps[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;ps[i+2].DescriptorTable={1,&ranges[i]};}
  D3D12_STATIC_SAMPLER_DESC sam[2]{};for(UINT i=0;i<2;++i){auto& s=sam[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxAnisotropy=1;s.ShaderRegister=i?11:6;s.RegisterSpace=1;}
  D3D12_ROOT_SIGNATURE_DESC rd{4,ps,2,sam,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob>b,e;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e));root.Reset();Check(gpu.device->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&root)));
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={kGIStableEnergyShader,sizeof(kGIStableEnergyShader)};temporal.Reset();Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&temporal)));
  pd.CS=old?D3D12_SHADER_BYTECODE{kGISpatialR375_0,sizeof(kGISpatialR375_0)}:D3D12_SHADER_BYTECODE{kGISteadySpatial0,sizeof(kGISteadySpatial0)};Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&passes[0])));
  pd.CS=old?D3D12_SHADER_BYTECODE{kGISpatialR375_1,sizeof(kGISpatialR375_1)}:D3D12_SHADER_BYTECODE{kGISteadySpatial1,sizeof(kGISteadySpatial1)};Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&passes[1])));
 }
 void Dispatch8(ID3D12PipelineState* pso,std::array<ID3D12Resource*,8> inputs,Tex& out,ID3D12Resource* cb0,ID3D12Resource* cb1){
  UINT first=slot;for(UINT i=0;i<9;++i){auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(slot++)*gpu.descriptorSize;
   if(i==8){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;gpu.device->CreateUnorderedAccessView(out.r.Get(),nullptr,&v,cpu);}
   else{D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;gpu.device->CreateShaderResourceView(inputs[i],&v,cpu);}}
  auto srv=gpu.views->GetGPUDescriptorHandleForHeapStart();srv.ptr+=UINT64(first)*gpu.descriptorSize;auto uav=srv;uav.ptr+=UINT64(8)*gpu.descriptorSize;
  gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetComputeRootConstantBufferView(0,cb0->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(1,cb1->GetGPUVirtualAddress());gpu.list->SetComputeRootDescriptorTable(2,srv);gpu.list->SetComputeRootDescriptorTable(3,uav);gpu.list->SetPipelineState(pso);gpu.list->Dispatch(W/8,H/8,1);gpu.Transition(out.r.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 }
};
static Stats RunChain(bool old,unsigned mode){
 Chain h(old);auto& g=h.gpu;
 Image flat(W*H,Pixel{.5f,.5f,.5f,.5f}),material(W*H,Pixel{0,0,1.f/255.f,0}),mv(W*H,Pixel{mode==1?.25f/W:0.f,0,0,0});
 auto depth=h.Texture(&flat),g1=h.Texture(&flat),g2=h.Texture(&material),motion=h.Texture(&mv);
 Image initial(W*H,Pixel{.3f,.3f,.3f,.3f});auto history=h.Texture(&initial);
 std::array<float,288> sys{};sys[0]=sys[4]=float(W);sys[1]=sys[5]=float(H);sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;
 auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
 std::array<UINT,64> filter{};filter[5]=2;filter[6]=mode==3?1:0;
 auto cbT=g.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
 ComPtr<ID3D12Resource> cbS[2];for(UINT p=0;p<2;++p){filter[4]=p;cbS[p]=g.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());}
 std::vector<Tex> frames;std::vector<ComPtr<ID3D12Resource>> reads;
 for(UINT f=0;f<N;++f){
  Image raw(W*H),hits(W*H,Pixel{f%2?1.f:0.f,0,0,0});
  for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){float v=.3f+.15f*std::cos(float(x)*.78539816f)*std::cos(float(y)*.78539816f);if(mode==2)v=.4f;if(mode==3)v=(x==12&&y==8)?10.f:.1f;raw[y*W+x]={v,v,v,v};}
  auto current=h.Texture(&raw),hit=h.Texture(&hits),temp=h.Texture(),a=h.Texture(),b=h.Texture();
  h.Dispatch8(h.temporal.Get(),{depth.r.Get(),depth.r.Get(),motion.r.Get(),current.r.Get(),history.r.Get(),current.r.Get(),history.r.Get(),hit.r.Get()},temp,cb0.Get(),cbT.Get());
  h.Dispatch8(h.passes[0].Get(),{g1.r.Get(),g2.r.Get(),depth.r.Get(),depth.r.Get(),motion.r.Get(),temp.r.Get(),history.r.Get(),hit.r.Get()},a,cb0.Get(),cbS[0].Get());
  h.Dispatch8(h.passes[1].Get(),{g1.r.Get(),g2.r.Get(),depth.r.Get(),depth.r.Get(),motion.r.Get(),a.r.Get(),history.r.Get(),hit.r.Get()},b,cb0.Get(),cbS[1].Get());
  g.Transition(b.r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);auto read=g.Buffer(b.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=b.f;src.pResource=b.r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  g.Transition(b.r.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);history=b;frames.push_back(b);reads.push_back(read);
 }
 g.Submit();Stats s;Image previous(W*H);double count=0;
 for(UINT f=0;f<N;++f){void* data=nullptr;Check(reads[f]->Map(0,nullptr,&data));auto& t=frames[f];
  for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){auto* p=reinterpret_cast<const float*>(static_cast<const unsigned char*>(data)+t.f.Offset+y*t.f.Footprint.RowPitch)+x*4;
   Require(std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]),"chain nonfinite");Require(std::abs(p[0]-p[3])<1e-5f,"chain alpha ABI");
   if(f>=16&&(mode>=2||(x>=4&&x<W-4&&y>=4&&y<H-4))){s.delta+=(p[1]-previous[y*W+x][1])*(p[1]-previous[y*W+x][1]);s.mean+=p[1];++count;}
   previous[y*W+x]={p[0],p[1],p[2],p[3]};
  }reads[f]->Unmap(0,nullptr);
 }s.delta=std::sqrt(s.delta/count);s.mean/=count;return s;
}
int main(){try{for(unsigned mode=0;mode<4;++mode){auto old=RunChain(true,mode),next=RunChain(false,mode);
 std::cout<<"R376 full-chain mode="<<mode<<" old_delta="<<old.delta<<" new_delta="<<next.delta<<" old_mean="<<old.mean<<" new_mean="<<next.mean<<'\n';
 if(mode<2){Require(old.delta>.0001,"varying hit-distance did not reproduce pumping");Require(next.delta<old.delta*.25,"hit-distance pumping not suppressed");Require(std::abs(next.mean-old.mean)<.01,"chain mean brightness changed");}
 if(mode==2)Require(std::abs(next.mean-.4)<.0001,"flat light changed");
 if(mode==3)Require(std::abs((next.mean-.1)*W*H-9.9)<.005,"compact light energy lost");
 }std::cout<<"PASS R376 full temporal/spatial feedback, both spatial variants, varying hit-distance, flat and compact light preservation\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
