// Fractional reprojection across distinct previous-frame surfaces. Executes DXBC.
#define main OwnedSequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
#include "../build/gi_motion_history.generated.h"
#include "../build/gi_stable_r380.generated.h"
static float Footprint(bool old,unsigned geometry,float fraction,float vertical=0.f,float viewOffset=0.f,bool cut=false){
 Harness h(false);auto& g=h.gpu;
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=h.root.Get();
 pd.CS=old?D3D12_SHADER_BYTECODE{kGIStableR380,sizeof(kGIStableR380)}:D3D12_SHADER_BYTECODE{kGIStableEnergyShader,sizeof(kGIStableEnergyShader)};
 h.temporal.Reset();Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&h.temporal)));
 Image depth(W*H,Pixel{.5f,.7f,.9f,1}),pastDepth(W*H),color(W*H),past(W*H),mv(W*H,Pixel{fraction/W,vertical/H,0,0});
 for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){
  bool right=x>W/2;float c=right?1.f:.4f,p=right?1.f:.2f;
  float z=.5f;
  if(geometry==0&&right)z=.508f; // Only dark history belongs to receiver.
  if(geometry==1)z=right?.508f:.492f; // Average matches; neither tap matches.
  if(geometry==3&&!right)z=.508f; // Only bright history belongs to receiver.
  if(geometry==4){const bool valid=x<=W/2&&y<=H/2;z=valid?.5f:.508f;p=valid?.2f:1.f;}
  if(geometry==5){const bool valid=x>W/2&&y>H/2;z=valid?.5f:.508f;p=valid?1.f:.2f;}
  pastDepth[y*W+x]={z-viewOffset,.7f,.9f,1};color[y*W+x]={c,c,c,c};past[y*W+x]={p,p,p,p};
 }
 auto d=h.Texture(&depth),pdtex=h.Texture(&pastDepth),motion=h.Texture(&mv),current=h.Texture(&color),history=h.Texture(&past),out=h.Texture();
 std::array<float,192> sys{};sys[0]=sys[4]=float(W);sys[1]=sys[5]=float(H);sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;sys[150]=viewOffset;
 auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
 std::array<UINT,64> filter{};filter[6]=cut?1u:0u;auto cb1=g.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
 h.Dispatch(h.temporal.Get(),{d.r.Get(),pdtex.r.Get(),motion.r.Get(),current.r.Get(),history.r.Get(),current.r.Get()},out,cb0.Get(),cb1.Get());
 g.Transition(out.r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 auto read=g.Buffer(out.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=out.f;src.pResource=out.r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);g.Submit();
 void* data=nullptr;Check(read->Map(0,nullptr,&data));auto* row=reinterpret_cast<const float*>(static_cast<const unsigned char*>(data)+out.f.Offset+(H/2)*out.f.Footprint.RowPitch);
 float result=row[4*(W/2)];Require(std::isfinite(result)&&std::abs(result-row[4*(W/2)+3])<1e-5f,"history footprint finite/red-alpha ABI");read->Unmap(0,nullptr);return result;
}
int main(){try{
 float oldMin=100,oldMax=-100,newMin=100,newMax=-100;
 for(float phase:{.25f,.5f,.75f}){
  float old=Footprint(true,0,phase),next=Footprint(false,0,phase);
  std::cout<<"History footprint phase="<<phase<<" R380="<<old<<" validated="<<next<<'\n';
  oldMin=std::min(oldMin,old);oldMax=std::max(oldMax,old);newMin=std::min(newMin,next);newMax=std::max(newMax,next);
  Require(std::abs(next-.305f)<.0001f,"valid dark history contaminated by foreign bright surface");
 }
 Require(oldMax-oldMin>.1f,"control failed to reproduce fractional footprint fluctuation");
 Require(newMax-newMin<.0001f,"validated footprint still fluctuates");
 const float rejected=Footprint(false,1,.5f),valid=Footprint(false,2,.5f),bright=Footprint(false,3,.5f);
 std::cout<<"History footprint rejected="<<rejected<<" continuous="<<valid<<" bright="<<bright<<'\n';
 Require(std::abs(rejected-.55f)<.0001f,"averaged depth admitted invalid history");
 Require(std::abs(valid-Footprint(true,2,.5f))<.0001f,"continuous surface interpolation changed");
 Require(std::abs(bright-.865f)<.0001f,"valid bright history lost energy");
 for(float vertical:{.25f,.5f,.75f}){
  Require(std::abs(Footprint(false,4,.5f,vertical)-.305f)<.0001f,"four-tap dark corner normalization failed");
  Require(std::abs(Footprint(false,5,.5f,vertical)-.865f)<.0001f,"four-tap bright corner normalization failed");
 }
 Require(std::abs(Footprint(false,0,.5f,0.f,.1f)-.305f)<.0001f,"native previous-view depth offset lost");
 Require(std::abs(Footprint(false,0,.5f,0.f,0.f,true)-.55f)<.0001f,"camera cut retained history");
 std::cout<<"PASS validated history footprint: subpixel phases, invalid average, valid interpolation, bright retention\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
