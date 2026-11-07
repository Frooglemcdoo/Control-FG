// Surface-history rejection using actual shipped bytecode and an archived control.
#define main OwnedSequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
#include "../build/gi_motion_history.generated.h"
#include "fixtures/gi-stable-r378.h"
static float EdgeHistory(bool old,bool separateSurface){
 Harness h(false);auto& g=h.gpu;
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=h.root.Get();
 pd.CS=old?D3D12_SHADER_BYTECODE{kGIStableR378,sizeof(kGIStableR378)}:D3D12_SHADER_BYTECODE{kGIStableEnergyShader,sizeof(kGIStableEnergyShader)};
 h.temporal.Reset();Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&h.temporal)));
 Image geometry(W*H),color(W*H),zero(W*H,Pixel{0,0,0,0}),past(W*H,Pixel{10,10,10,10});
 for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){
  const bool right=x>=W/2;const float value=right?10.f:.2f;
  geometry[y*W+x]={separateSurface&&right?.8f:.2f,.5f,.7f,1.f};
  color[y*W+x]={value,value,value,value};
 }
 auto depth=h.Texture(&geometry),motion=h.Texture(&zero),current=h.Texture(&color),history=h.Texture(&past),out=h.Texture();
 std::array<float,192> sys{};sys[0]=sys[4]=float(W);sys[1]=sys[5]=float(H);sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;
 auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
 std::array<UINT,64> filter{};auto cb1=g.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
 h.Dispatch(h.temporal.Get(),{depth.r.Get(),depth.r.Get(),motion.r.Get(),current.r.Get(),history.r.Get(),current.r.Get()},out,cb0.Get(),cb1.Get());
 g.Transition(out.r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 auto read=g.Buffer(out.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=out.f;src.pResource=out.r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);g.Submit();
 void* data=nullptr;Check(read->Map(0,nullptr,&data));auto* row=reinterpret_cast<const float*>(static_cast<const unsigned char*>(data)+out.f.Offset+(H/2)*out.f.Footprint.RowPitch);
 float edge=row[4*(W/2-1)];Require(std::abs(row[4*(W/2+3)]-10.f)<.0001f,"valid bright surface lost light");
 read->Unmap(0,nullptr);return edge;
}
int main(){try{
 const float old=EdgeHistory(true,true),next=EdgeHistory(false,true),continuous=EdgeHistory(false,false);
 std::cout<<"R380 edge history control="<<old<<" corrected="<<next<<" continuous_surface="<<continuous<<'\n';
 Require(old>7.f,"control did not reproduce cross-surface history admission");
 Require(std::abs(next-.2357f)<.0001f,"stale cross-surface light not rejected");
 Require(continuous>7.f,"valid continuous-surface history was rejected");
 std::cout<<"PASS R380 geometry-gated history bounds, bright-light retention, effective X depth\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
