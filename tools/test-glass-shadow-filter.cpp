#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
using Microsoft::WRL::ComPtr;
static void check(bool ok){if(!ok){puts("FAIL: glass stable shadow filter");std::exit(1);}}
int main(){
 const char* source=R"(
#include "pt_glass_shadow_filter.hlsli"
Texture2D<float4> atlas:register(t0);
RWTexture2D<float4> output:register(u0);
[numthreads(1,1,1)] void main(uint3 id:SV_DispatchThreadID){
 float2 uv=float2((12.5f+float(id.x)*.25f)/64.0f,.5f);
 if(id.x==33)uv.x=0.0f;
 if(id.x==34)uv.x=.5f;
 float v=PTGlassStableShadow(atlas,uv,.5f,.05f,float4(0,0,.5,1));
 float repeated=PTGlassStableShadow(atlas,uv,.5f,.05f,float4(0,0,.5,1));
 // At the boundary between depth 0 and 1, compare-first interpolation
 // must yield fractional visibility, not a binary depth comparison.
 float edge=PTGlassShadowCompare(atlas,float2(.25f,.5f),.5f,float2(64,32),int2(0,0),int2(31,31));
 output[id.xy]=float4(v,repeated,edge,1);
})";
 ComPtr<ID3D11Device> d;ComPtr<ID3D11DeviceContext> c;
 check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)));
 ComPtr<ID3DBlob> code,error;
 HRESULT hr=D3DCompile(source,strlen(source),"src/shaders/glass-shadow-test.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
 if(error)fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);check(SUCCEEDED(hr));
 ComPtr<ID3D11ComputeShader> cs;check(SUCCEEDED(d->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&cs)));
 D3D11_TEXTURE2D_DESC desc{};desc.Width=35;desc.Height=1;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
 ComPtr<ID3D11Texture2D> target,readback;check(SUCCEEDED(d->CreateTexture2D(&desc,nullptr,&target)));
 ComPtr<ID3D11UnorderedAccessView> uav;check(SUCCEEDED(d->CreateUnorderedAccessView(target.Get(),nullptr,&uav)));
 desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(SUCCEEDED(d->CreateTexture2D(&desc,nullptr,&readback)));
 // Left atlas tile contains a hard shadow edge; neighboring tile is dark.
 float pixels[64*32*4]{};
 for(unsigned y=0;y<32;++y)for(unsigned x=16;x<32;++x)pixels[(y*64+x)*4]=1.0f;
 D3D11_TEXTURE2D_DESC ad{};ad.Width=64;ad.Height=32;ad.MipLevels=ad.ArraySize=1;ad.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;ad.SampleDesc.Count=1;ad.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA init{};init.pSysMem=pixels;init.SysMemPitch=64*4*sizeof(float);
 ComPtr<ID3D11Texture2D> atlas;check(SUCCEEDED(d->CreateTexture2D(&ad,&init,&atlas)));
 ComPtr<ID3D11ShaderResourceView> srv;check(SUCCEEDED(d->CreateShaderResourceView(atlas.Get(),nullptr,&srv)));
 ID3D11ShaderResourceView* inputs[]={srv.Get()};c->CSSetShaderResources(0,1,inputs);
 c->CSSetShader(cs.Get(),nullptr,0);ID3D11UnorderedAccessView* views[]={uav.Get()};c->CSSetUnorderedAccessViews(0,1,views,nullptr);c->Dispatch(35,1,1);
 ID3D11UnorderedAccessView* none[]={nullptr};c->CSSetUnorderedAccessViews(0,1,none,nullptr);c->CopyResource(readback.Get(),target.Get());
 D3D11_MAPPED_SUBRESOURCE mapped{};check(SUCCEEDED(c->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
 const float* actual=static_cast<const float*>(mapped.pData);
 unsigned fractional=0;
 for(unsigned i=0;i<35;++i){
   float v=actual[4*i];check(std::isfinite(v)&&v>=0&&v<=1);
   check(v==actual[4*i+1]);check(std::abs(actual[4*i+2]-.5f)<.00001f);
   if(i<=32){
     if(v>0&&v<1)++fractional;
     if(i>0){check(v>=actual[4*(i-1)]-.00001f);check(v-actual[4*(i-1)]<.16f);}
   }
 }
 check(actual[0]==0&&actual[32*4]==1&&fractional>=8);
 check(actual[33*4]==0&&actual[34*4]==1); // Atlas boundaries cannot leak.
 c->Unmap(readback.Get(),0);
 puts("PASS: production glass shadow filter on WARP: lit/dark, compare-first interpolation, smooth subtexel motion, repeatability and atlas tile isolation.");
}
