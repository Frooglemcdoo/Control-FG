#include "gpu_test_device.h"
#include "../src/gi_reuse/guiding_contribution.h"
#include "../build/gi_candidate_fixture.h"
#include <limits>
using namespace gi_reuse;
struct Ray {GIPoint throughput;uint material;GIPoint secondary;uint index;float nativePdf,guidedPdf;uint frame,generation;};
struct Pixel {uint count;float scale;uint frame,generation;Ray rays[64];};
struct Output {float total[3];uint valid;float surface[3];uint hits;float miss[3];uint misses;float training;uint count,reserved[2];};
static_assert(sizeof(Ray)==48&&sizeof(Pixel)==3088&&sizeof(Output)==64);
float F(uint bits){float f;std::memcpy(&f,&bits,4);return f;}
bool Close(double a,double b){return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=1e-6+2e-5*std::abs(b);}
int main()try{
 constexpr uint fixtureStride=520,fixtureCount=sizeof(fixtureWords)/sizeof(uint)/fixtureStride;
 std::vector<Pixel> pixels;
 // Ratios are synthetic; throughput and secondary lighting are trace-derived.
 // Each fixture also runs with unit correction and a legitimate zero sample.
 for(uint mode=0;mode<3;++mode)for(uint f=0;f<fixtureCount;++f){
  const auto* src=fixtureWords+f*fixtureStride;Pixel p{};p.count=src[0];p.scale=F(src[1]);p.frame=100;p.generation=27;
  for(uint i=0;i<p.count;++i){const auto* s=src+4+i*8;Ray r{};r.throughput={F(s[0]),F(s[1]),F(s[2])};r.material=s[3];r.secondary={F(s[4]),F(s[5]),F(s[6])};r.index=i;r.frame=p.frame;r.generation=p.generation;
   r.nativePdf=.125f;r.guidedPdf=mode==1?(i%3==0?.0625f:i%3==1?.5f:.125f):.125f;
   if(mode==2&&i==0)r.nativePdf=0;p.rays[i]=r;
  }pixels.push_back(p);
 }
 const uint validCount=static_cast<uint>(pixels.size());
 for(uint k=0;k<10;++k){auto p=pixels[0];auto& r=p.rays[0];
  if(k==0)--r.frame;if(k==1)++r.generation;if(k==2)++r.index;if(k==3)r.guidedPdf=0;
  if(k==4)r.nativePdf=-1;if(k==5)r.secondary.x=std::numeric_limits<float>::quiet_NaN();
  if(k==6)r.material=65534;if(k==7)p.count=65;if(k==8)p.count=0;
  if(k==9){r.throughput.x=std::numeric_limits<float>::max();r.secondary.x=4;}
  pixels.push_back(p);
 }
 Device gpu(false);auto input=gpu.Upload(pixels);const uint count=static_cast<uint>(pixels.size());
 std::vector<unsigned char> guard((count+1)*sizeof(Output),0xcd);auto init=gpu.Upload(guard);
 auto output=gpu.Buffer(guard.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
 gpu.list->CopyResource(output.Get(),init.Get());gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER params[2]{};for(uint i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,&ranges[i]};}
 D3D12_ROOT_SIGNATURE_DESC rd{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob>b,e;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e));
 ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-guided-accounting.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());gpu.list->SetComputeRootDescriptorTable(0,gpu.View(input.Get(),0,sizeof(uint),count*(sizeof(Pixel)/sizeof(uint)),false));gpu.list->SetComputeRootDescriptorTable(1,gpu.View(output.Get(),1,sizeof(Output),count+1,true));gpu.list->Dispatch((count+63)/64+1,1,1);
 gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);auto read=gpu.Buffer(guard.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);gpu.list->CopyResource(read.Get(),output.Get());gpu.Submit();
 Output* out=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&out)));bool missSensitive=false,zeroSensitive=false;double maxNativeError=0;
 for(uint p=0;p<count;++p){const auto& in=pixels[p];const auto& got=out[p];
  if(p>=validCount){Output zero{};Require(!memcmp(&got,&zero,sizeof(zero)),"Invalid accounting packet admitted");continue;}
  Require(got.valid&&got.count==in.count,"Sample denominator changed");double total[3]{},surface[3]{},miss[3]{},training=0;uint hits=0,misses=0;
  for(uint i=0;i<in.count;++i){const auto& r=in.rays[i];double ratio=double(r.nativePdf)/r.guidedPdf;const double tp[]={r.throughput.x,r.throughput.y,r.throughput.z},li[]={r.secondary.x,r.secondary.y,r.secondary.z},lum[]={.2126,.7152,.0722};
   auto checked=GIApplyGuideCorrection(r.throughput,r.secondary,r.nativePdf,r.guidedPdf);Require(checked.valid,"CPU bridge rejected valid captured contribution");
   if(r.material==65535)++misses;else ++hits;
   for(uint c=0;c<3;++c){double v=tp[c]*li[c]*ratio;total[c]+=v;(r.material==65535?miss:surface)[c]+=v;training+=lum[c]*v;
    if(r.material==65535&&ratio!=1&&tp[c]*li[c]>1e-5)missSensitive=true;
    if(ratio==0&&tp[c]*li[c]>1e-5)zeroSensitive=true;
   }
  }
  Require(got.hits==hits&&got.misses==misses,"Hit/miss classification changed");Require(Close(got.training,training/in.count),"Training differs from weighted pre-fade contribution");
  for(uint c=0;c<3;++c){Require(Close(got.total[c],in.scale*total[c]/in.count)&&Close(got.surface[c],in.scale*surface[c]/in.count)&&Close(got.miss[c],in.scale*miss[c]/in.count),"Guided surface/miss energy mismatch");
   if(p<fixtureCount){double expected=F(fixtureWords[p*fixtureStride+516+c]);maxNativeError=std::max(maxNativeError,std::abs(got.total[c]-expected));Require(Close(got.total[c],expected),"Unit correction does not recover captured native RGB");}
  }
 }
 Require(missSensitive&&zeroSensitive,"Fixture cannot expose miss-weight or zero-count bugs");Require(!memcmp(out+count,guard.data()+count*sizeof(Output),sizeof(Output)),"Output guard changed");read->Unmap(0,nullptr);
 std::cout<<"PASS captured accounting bridge: "<<fixtureCount<<" fixtures x 3 corrections, native RGB max error="<<maxNativeError<<", weighted surface AND miss, zero terms retained, pre-fade learning, 10 invalid packets, output guard\n";
 std::cout<<"LIMIT: synthetic PDF ratios over captured accounting values; no new rays, full light replay, live bindings, hit-distance policy or FPS validation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
