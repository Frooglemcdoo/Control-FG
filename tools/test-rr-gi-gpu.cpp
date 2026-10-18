#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
using Microsoft::WRL::ComPtr;
static void check(bool v,const char* msg){if(!v){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
static void hr(HRESULT v){check(SUCCEEDED(v),"D3D operation");}
struct Pixel{float x,y,z,w;};
int main(){
 const UINT W=32,H=32;
 ComPtr<ID3D11Device> dev;ComPtr<ID3D11DeviceContext> ctx;hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx));
 std::ifstream f("src/shaders/rr_gi_temporal.hlsl");std::string source((std::istreambuf_iterator<char>(f)),{});
 for(size_t p=source.find(", space1");p!=std::string::npos;p=source.find(", space1"))source.erase(p,8);
 ComPtr<ID3DBlob> code,error;HRESULT result=D3DCompile(source.data(),source.size(),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
 if(error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);hr(result);
 ComPtr<ID3D11ComputeShader> shader;hr(dev->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader));
 D3D11_TEXTURE2D_DESC td{};td.Width=W;td.Height=H;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
 ComPtr<ID3D11Texture2D> output,read;hr(dev->CreateTexture2D(&td,nullptr,&output));ComPtr<ID3D11UnorderedAccessView> uav;hr(dev->CreateUnorderedAccessView(output.Get(),nullptr,&uav));td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;hr(dev->CreateTexture2D(&td,nullptr,&read));
 D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;ComPtr<ID3D11SamplerState> sampler;hr(dev->CreateSamplerState(&sd,&sampler));
 auto run=[&](float history,float velocity,float prevDepth,unsigned cut,bool noise){
  ComPtr<ID3D11Texture2D> textures[5];ComPtr<ID3D11ShaderResourceView> srvs[5];
  for(UINT slot=0;slot<5;++slot){std::vector<Pixel> data(W*H);
   for(UINT i=0;i<W*H;++i){float v=slot==0?10.0f:slot==1?prevDepth:slot==2?0.0f:slot==3?1.0f:history;
    if(slot==3&&noise)v=((i%W+i/W)%2)?1.2f:0.8f;
    data[i]={v,v,v,0};if(slot==2)data[i]={velocity,0,0,0};}
   D3D11_TEXTURE2D_DESC d=td;d.Usage=D3D11_USAGE_IMMUTABLE;d.CPUAccessFlags=0;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
   D3D11_SUBRESOURCE_DATA init{data.data(),W*sizeof(Pixel),0};hr(dev->CreateTexture2D(&d,&init,&textures[slot]));hr(dev->CreateShaderResourceView(textures[slot].Get(),nullptr,&srvs[slot]));
  }
  float sys[152]{};sys[2]=sys[3]=sys[6]=sys[7]=1.0f/32.0f;unsigned filter[8]{};filter[6]=cut;
  ComPtr<ID3D11Buffer> cb[2];for(UINT i=0;i<2;++i){D3D11_BUFFER_DESC d{};d.ByteWidth=UINT(i?sizeof(filter):sizeof(sys));d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;D3D11_SUBRESOURCE_DATA init{i?static_cast<void*>(filter):static_cast<void*>(sys),0,0};hr(dev->CreateBuffer(&d,&init,&cb[i]));}
  ID3D11ShaderResourceView* views[5];for(UINT i=0;i<5;++i)views[i]=srvs[i].Get();ID3D11Buffer* buffers[]={cb[0].Get(),cb[1].Get()};auto* target=uav.Get();auto* samp=sampler.Get();
  ctx->CSSetShader(shader.Get(),nullptr,0);ctx->CSSetShaderResources(0,5,views);ctx->CSSetConstantBuffers(0,2,buffers);ctx->CSSetSamplers(6,1,&samp);ctx->CSSetSamplers(11,1,&samp);ctx->CSSetUnorderedAccessViews(0,1,&target,nullptr);ctx->Dispatch(4,4,1);
  ID3D11UnorderedAccessView* nullUav=nullptr;ctx->CSSetUnorderedAccessViews(0,1,&nullUav,nullptr);ctx->CopyResource(read.Get(),output.Get());D3D11_MAPPED_SUBRESOURCE map{};hr(ctx->Map(read.Get(),0,D3D11_MAP_READ,0,&map));Pixel p{};std::memcpy(&p,static_cast<const char*>(map.pData)+16*map.RowPitch+16*sizeof(Pixel),sizeof(p));ctx->Unmap(read.Get(),0);return p.x;
 };
 auto near=[](float a,float b){return std::isfinite(a)&&std::abs(a-b)<0.0001f;};
 check(near(run(1.04f,0,10,0,false),1.014f),"static 35 percent blend");
 check(near(run(1.04f,0,10,1,false),1.0f),"camera cut rejects history");
 check(near(run(1.04f,0.25f,10,0,false),1.0f),"eight-pixel motion rejects history");
 check(near(run(1.04f,0,20,0,false),1.0f),"depth mismatch rejects history");
 check(near(run(100,0,10,0,false),1.0175f),"stale bright history bounded");
 check(near(run(1,0,10,0,true),0.87f),"stationary stochastic error reduced");
 std::puts("PASS R16 production GPU: static stabilization, camera cuts, motion/depth rejection, stale-light clamp");
}
