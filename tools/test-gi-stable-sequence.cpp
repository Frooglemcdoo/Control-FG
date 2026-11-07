// Compare actual R373 and R374 Stable temporal bytecode over 32 feedback frames.
// Spatial stage deliberately excluded: isolate the changed production stage.
#define main GISequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
#include "../build/gi_motion_history.generated.h"
#include "fixtures/gi-stable-r373.h"
static Stats StableRun(bool old,unsigned mode){
 Harness h(false);auto& g=h.gpu;
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=h.root.Get();
 pd.CS=old?D3D12_SHADER_BYTECODE{kGIStableR373,sizeof(kGIStableR373)}:D3D12_SHADER_BYTECODE{kGIStableEnergyShader,sizeof(kGIStableEnergyShader)};
 h.temporal.Reset();Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&h.temporal)));
 Image flat(W*H,Pixel{.5f,.5f,.5f,.5f}),mv(W*H,Pixel{mode==1?.25f/W:0.f,0,0,0});
 auto depth=h.Texture(&flat),g1=h.Texture(&flat),g2=h.Texture(&flat),motion=h.Texture(&mv);
 Image initial(W*H,Pixel{.2f,.3f,.1f,.2f});auto history=h.Texture(&initial);
 std::array<float,192> sys{};sys[0]=sys[4]=float(W);sys[1]=sys[5]=float(H);sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;
 auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
 std::array<UINT,64> filter{};if(mode==2)filter[6]=1;auto cb1=g.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
 std::vector<Tex> frames;std::vector<ComPtr<ID3D12Resource>> reads;
 for(UINT f=0;f<N;++f){
  Image raw(W*H);
  for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){
   UINT id=y*W+x;UINT bits=(id*1664525u+f*1013904223u+17u);bits^=bits>>16;bits*=2246822519u;bits^=bits>>13;
   float noise=float(bits&65535)/65535.f-.5f;
   raw[id]=Pixel{.3f+noise*.58f,.3f+noise*.58f,.3f+noise*.58f,0};
   if(mode==2)raw[id]=Pixel{.1f,.1f,.1f,0};
   if(mode==2&&y==8&&x==12)raw[id]=Pixel{10,10,10,0};
  }
  auto current=h.Texture(&raw),temporal=h.Texture(),spatial=h.Texture();
  h.Dispatch(h.temporal.Get(),{depth.r.Get(),depth.r.Get(),motion.r.Get(),current.r.Get(),history.r.Get(),current.r.Get()},temporal,cb0.Get(),cb1.Get());
  spatial=temporal;
  g.Transition(spatial.r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
  auto read=g.Buffer(spatial.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=spatial.f;src.pResource=spatial.r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  g.Transition(spatial.r.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  history=spatial;frames.push_back(spatial);reads.push_back(read);
 }
 g.Submit();Stats s;Image previous(W*H);double count=0;
 for(UINT f=0;f<N;++f){void* data=nullptr;Check(reads[f]->Map(0,nullptr,&data));auto& t=frames[f];
  for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){auto* p=reinterpret_cast<const float*>(static_cast<const unsigned char*>(data)+t.f.Offset+y*t.f.Footprint.RowPitch)+x*4;
   Require(std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]),"nonfinite sequence output");Require(std::abs(p[0]-p[3])<1e-5f,"red/alpha ABI");

   if(f>=4){s.error+=(p[1]-.3)*(p[1]-.3);s.delta+=(p[1]-previous[y*W+x][1])*(p[1]-previous[y*W+x][1]);s.mean+=p[1];s.maxValue=std::max(s.maxValue,p[1]);++count;}
   previous[y*W+x]={p[0],p[1],p[2],p[3]};
  }reads[f]->Unmap(0,nullptr);
 }
 s.error=std::sqrt(s.error/count);s.delta=std::sqrt(s.delta/count);s.mean/=count;return s;
}
int main(){try{
 for(unsigned mode=0;mode<3;++mode){auto before=StableRun(true,mode),after=StableRun(false,mode);
  std::cout<<"R374 Stable mode="<<mode<<" old_delta="<<before.delta<<" new_delta="<<after.delta<<" old_rmse="<<before.error<<" new_rmse="<<after.error<<" old_mean="<<before.mean<<" new_mean="<<after.mean<<'\n';
  if(mode<2){Require(after.delta<before.delta*.8,"stable noise in motion did not improve");Require(after.error<before.error,"stable error did not improve");Require(std::abs(after.mean-.3)<.03,"mean diffuse energy changed");}
  else Require(std::abs((after.mean-.1)*W*H-9.9)<.001,"compact light total energy lost");
 }
 std::cout<<"PASS R374 Stable stationary/moving noise feedback and compact-light energy\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
