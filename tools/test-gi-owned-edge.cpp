// The foreground has valid history; independently moving background must not
// veto it. Run the actual temporal + spatial bytecode with 32-frame feedback.
#define main GISequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
static Stats EdgeRun(bool old,unsigned mode){
 Harness h(false);auto& g=h.gpu;
 if(old){ComPtr<ID3DBlob>b,e;Check(D3DCompileFromFile(L"tools/fixtures/gi-owned-r371.hlsl",nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&e));D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=h.root.Get();pd.CS={b->GetBufferPointer(),b->GetBufferSize()};h.temporal.Reset();Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&h.temporal)));}
 Image flat(W*H,Pixel{.5f,.5f,.5f,.5f}),mv(W*H,Pixel{mode==1?1.f/W:0.f,0,0,0});
 for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){
  flat[y*W+x][0]=x<16?.5f:1.f;
  mv[y*W+x]={x<16?(mode==1?.25f/W:0.f):-2.f/W,0,0,0};
 }
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
   raw[id]=off?Pixel{.02f,.03f,.01f,.02f}:Pixel{.2f+noise*.04f,.3f+noise*.04f,.1f+noise*.04f,0};
   // Two adjacent green fireflies, a different location every frame.
   const UINT peak=4+(f*7)%24;
   if(false && peak)raw[id]=Pixel{4,30,2,4};
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
  for(UINT y=2;y<H-2;++y)for(UINT x=15;x<16;++x){auto* p=reinterpret_cast<const float*>(static_cast<const unsigned char*>(data)+t.f.Offset+y*t.f.Footprint.RowPitch)+x*4;
   Require(std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]),"nonfinite sequence output");Require(std::abs(p[0]-p[3])<1e-5f,"red/alpha ABI");
   if(mode==2&&f>=24)s.lightOff=std::max(s.lightOff,std::abs(p[1]-.03f));
   if(f>=4 && (mode!=2||f<24)){s.error+=(p[1]-.3)*(p[1]-.3);s.delta+=(p[1]-previous[y*W+x][1])*(p[1]-previous[y*W+x][1]);s.mean+=p[1];s.maxValue=std::max(s.maxValue,p[1]);++count;}
   previous[y*W+x]={p[0],p[1],p[2],p[3]};
  }reads[f]->Unmap(0,nullptr);
 }
 s.error=std::sqrt(s.error/count);s.delta=std::sqrt(s.delta/count);s.mean/=count;return s;
}
int main(){try{
 for(unsigned mode=0;mode<2;++mode){auto before=EdgeRun(true,mode),after=EdgeRun(false,mode);
  std::cout<<"R372 edge mode="<<mode<<" old_delta="<<before.delta<<" new_delta="<<after.delta<<" old_rmse="<<before.error<<" new_rmse="<<after.error<<" mean="<<after.mean<<'\n';
  Require(after.delta<before.delta*.85,"valid foreground edge history still discarded");
  Require(after.error<before.error,"foreground edge noise did not improve");
  Require(std::abs(after.mean-.3)<.005,"foreground edge brightness changed");
 }
 std::cout<<"PASS R372 stationary and subpixel-moving foreground history beside unrelated background depth/motion\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
