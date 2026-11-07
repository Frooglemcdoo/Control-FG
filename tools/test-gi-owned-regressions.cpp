// Actual bytecode regression cases absent from the original flat-noise fixture.
#define main GISequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
static Image Evaluate(bool prior,const Image& raw,const Image& previous,const Image& vectors,bool cut){
 Harness h(false);auto& g=h.gpu;
 if(prior){ComPtr<ID3DBlob>b,e;Check(D3DCompileFromFile(L"tools/fixtures/gi-owned-r369.hlsl",nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&e));D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=h.root.Get();pd.CS={b->GetBufferPointer(),b->GetBufferSize()};h.temporal.Reset();Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&h.temporal)));}
 Image flat(W*H,Pixel{.5f,.5f,.5f,.5f});auto depth=h.Texture(&flat),g1=h.Texture(&flat),g2=h.Texture(&flat),motion=h.Texture(&vectors),current=h.Texture(&raw),history=h.Texture(&previous),temporal=h.Texture(),spatial=h.Texture();
 std::array<float,192> sys{};sys[0]=sys[4]=float(W);sys[1]=sys[5]=float(H);sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;
 auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());std::array<UINT,64> filter{};filter[6]=cut?1:0;auto cb1=g.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
 h.Dispatch(h.temporal.Get(),{depth.r.Get(),depth.r.Get(),motion.r.Get(),current.r.Get(),history.r.Get(),current.r.Get()},temporal,cb0.Get(),cb1.Get());
 h.Dispatch(h.spatial.Get(),{g1.r.Get(),g2.r.Get(),depth.r.Get(),depth.r.Get(),motion.r.Get(),temporal.r.Get()},spatial,cb0.Get(),cb1.Get());
 g.Transition(spatial.r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);auto read=g.Buffer(spatial.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=spatial.f;src.pResource=spatial.r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);g.Submit();void* data=nullptr;Check(read->Map(0,nullptr,&data));Image result(W*H);
 for(UINT y=0;y<H;++y)std::memcpy(result.data()+y*W,static_cast<const unsigned char*>(data)+spatial.f.Offset+y*spatial.f.Footprint.RowPitch,W*sizeof(Pixel));read->Unmap(0,nullptr);return result;
}
int main(){try{
 Image base(W*H,Pixel{.1f,.1f,.1f,.1f}),zeroMotion(W*H,Pixel{0,0,0,0});
 for(unsigned width: {1u,2u,6u}){
  Image current=base;for(UINT x=12;x<12+width;++x)current[8*W+x]={10,10,10,10};
  auto old=Evaluate(true,current,base,zeroMotion,true),now=Evaluate(false,current,base,zeroMotion,true);
  double before=0,after=0;for(UINT i=0;i<W*H;++i){before+=old[i][0]-.1;after+=now[i][0]-.1;}
  const double expected=9.9*width;
  std::cout<<"R370 compact light width="<<width<<" R369_energy="<<before<<" R370_energy="<<after<<" expected="<<expected<<'\n';
  Require(std::abs(after/expected-1)<.001,"compact current light energy removed");
  if(width<=2)Require(before<expected*.1,"fixture did not reproduce R369 light loss");
  auto accumulated=base;for(unsigned f=0;f<8;++f)accumulated=Evaluate(false,current,accumulated,zeroMotion,false);
  double retained=0;for(const auto& px:accumulated)retained+=px[0]-.1;
  std::cout<<"R370 persistent compact light energy="<<retained<<'\n';
  Require(retained>expected*.95,"persistent compact light fades in feedback");
 }
 Image current=base,previous=base,motion=zeroMotion;
 for(UINT y=3;y<13;++y){for(UINT x=10;x<18;++x)previous[y*W+x]={1,1,1,1};for(UINT x=12;x<20;++x){current[y*W+x]={1,1,1,1};motion[y*W+x]={-2.f/W,0,0,0};}}
 auto reference=Evaluate(false,current,previous,motion,true),old=Evaluate(true,current,previous,motion,false),now=Evaluate(false,current,previous,motion,false);
 double oldTrail=0,newTrail=0;for(UINT y=5;y<11;++y)for(UINT x=8;x<13;++x){oldTrail+=std::abs(old[y*W+x][0]-reference[y*W+x][0]);newTrail+=std::abs(now[y*W+x][0]-reference[y*W+x][0]);}
 std::cout<<"R370 moving silhouette R369_trail="<<oldTrail<<" R370_trail="<<newTrail<<'\n';
 Require(oldTrail>.01,"fixture did not expose old history trail");Require(newTrail<oldTrail*.15,"moving silhouette history trail remains");
 std::cout<<"PASS R370 compact light energy, persistent illumination, moving silhouette rejection\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
