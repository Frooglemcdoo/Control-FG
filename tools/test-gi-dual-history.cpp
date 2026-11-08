#define main OwnedSequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
#include "../build/gi_dual_history.generated.h"
#include "../build/gi_motion_history.generated.h"
struct Dual:Harness {
 ComPtr<ID3D12PipelineState> baseline;
 Dual():Harness(false){
  D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,7,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,0}};
  D3D12_ROOT_PARAMETER ps[4]{};for(UINT i=0;i<2;++i){ps[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;ps[i].Descriptor.ShaderRegister=i;ps[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;ps[i+2].DescriptorTable={1,&ranges[i]};}
  D3D12_STATIC_SAMPLER_DESC sam[2]{};for(UINT i=0;i<2;++i){auto&s=sam[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxAnisotropy=1;s.ShaderRegister=i?11:6;s.RegisterSpace=1;}
  D3D12_ROOT_SIGNATURE_DESC rd{4,ps,2,sam,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob>b,e;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e));root.Reset();Check(gpu.device->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&root)));
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={kGIDualHistory,sizeof(kGIDualHistory)};temporal.Reset();Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&temporal)));pd.CS={kGIStableEnergyShader,sizeof(kGIStableEnergyShader)};Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&baseline)));
 }
 void Run(bool old,const std::array<ID3D12Resource*,7>& inputs,Tex& out,Tex& fast,Tex& moments,ID3D12Resource* cb0,ID3D12Resource* cb1){
  Require(slot+10<=2048,"descriptor capacity");UINT first=slot;
  for(UINT i=0;i<10;++i){auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(slot++)*gpu.descriptorSize;
   if(i<7){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;gpu.device->CreateShaderResourceView(inputs[i],&v,cpu);}
   else {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;gpu.device->CreateUnorderedAccessView(i==7?out.r.Get():i==8?fast.r.Get():moments.r.Get(),nullptr,&v,cpu);}
  }
  auto srv=gpu.views->GetGPUDescriptorHandleForHeapStart();srv.ptr+=UINT64(first)*gpu.descriptorSize;auto uav=srv;uav.ptr+=UINT64(7)*gpu.descriptorSize;
  gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetComputeRootConstantBufferView(0,cb0->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(1,cb1->GetGPUVirtualAddress());gpu.list->SetComputeRootDescriptorTable(2,srv);gpu.list->SetComputeRootDescriptorTable(3,uav);gpu.list->SetPipelineState(old?baseline.Get():temporal.Get());gpu.list->Dispatch(W/8,H/8,1);
  for(auto*t:{&out,&fast,&moments})gpu.Transition(t->r.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 }
};
struct Result {double delta=0,mean=0;float off=0,reveal=0;};
static Result Sequence(bool old,unsigned mode){
 Dual h;auto&g=h.gpu;Image zero(W*H,Pixel{}),flat(W*H,Pixel{.5f,.5f,.5f,.5f}),mv(W*H,Pixel{mode==3?.25f/W:0.f,0,0,0});
 auto depth=h.Texture(&flat),motion=h.Texture(&mv),history=h.Texture(&zero),fast=h.Texture(&zero),moments=h.Texture(&zero);
 std::array<float,192> sys{};sys[0]=sys[4]=W;sys[1]=sys[5]=H;sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;
 auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
 std::array<UINT,64> f{};auto cb=g.Buffer(sizeof(f),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,f.data());f[6]=1;auto cut=g.Buffer(sizeof(f),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,f.data());
 std::vector<Tex> outputs;std::vector<ComPtr<ID3D12Resource>> reads;
 for(UINT frame=0;frame<64;++frame){
  float value=mode==0||mode==3?(frame%2?.6f:.2f):.4f;
  if(mode==1&&frame>=40)value=.02f;
  if(mode==2&&frame>=40)value=4.f;
  if(mode==4&&frame>=40)value=.1f;
  Image raw(W*H,Pixel{value,value,value,value});auto current=h.Texture(&raw),out=h.Texture(),nf=h.Texture(),nm=h.Texture();
  auto previousDepth=depth;
  if(mode==4&&frame==40){Image different(W*H,Pixel{.9f,0,0,0});previousDepth=h.Texture(&different);}
  h.Run(old,{depth.r.Get(),previousDepth.r.Get(),motion.r.Get(),current.r.Get(),history.r.Get(),fast.r.Get(),moments.r.Get()},out,nf,nm,cb0.Get(),frame==0||mode==5&&frame==40?cut.Get():cb.Get());
  g.Transition(out.r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);auto read=g.Buffer(out.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=out.f;src.pResource=out.r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);g.Transition(out.r.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  history=out;fast=nf;moments=nm;outputs.push_back(out);reads.push_back(read);
 }
 g.Submit();Result r;float previous=0;
 for(UINT frame=0;frame<64;++frame){void* data=nullptr;Check(reads[frame]->Map(0,nullptr,&data));auto&t=outputs[frame];auto*p=reinterpret_cast<float*>(static_cast<unsigned char*>(data)+t.f.Offset+(H/2)*t.f.Footprint.RowPitch)+4*(W/2);float v=p[0];Require(std::isfinite(v)&&std::abs(p[3]-v)<1e-5f,"finite/red-alpha");reads[frame]->Unmap(0,nullptr);
  if(frame>=24&&frame<40){r.delta+=(v-previous)*(v-previous);r.mean+=v/16.;}if(frame==40)r.reveal=v;if(frame>=42)r.off=std::max(r.off,std::abs(v-(mode==1?.02f:mode==2?4.f:mode==4?.1f:.4f)));previous=v;
 }r.delta=std::sqrt(r.delta/16.);return r;
}
int main(){try{for(unsigned mode=0;mode<6;++mode){auto old=Sequence(true,mode),next=Sequence(false,mode);std::cout<<"Dual mode="<<mode<<" R382_delta="<<old.delta<<" dual_delta="<<next.delta<<" mean="<<next.mean<<" transition="<<next.reveal<<" settled_error="<<next.off<<'\n';
 if(mode==0||mode==3){Require(next.delta<old.delta*.25,"fluctuating-input suppression");Require(std::abs(next.mean-.4)<.015,"mean brightness drift");}
 if(mode==1||mode==2)Require(next.off<.005,"light transition lag");
 if(mode==4)Require(std::abs(next.reveal-.1)<1e-5,"revealed surface retained old light");
 if(mode==5)Require(std::abs(next.reveal-.4)<1e-5,"camera cut altered current light");
 }std::cout<<"PASS dual history candidate; runtime integration not enabled\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
