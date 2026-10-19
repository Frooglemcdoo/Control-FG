#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
using Microsoft::WRL::ComPtr;
static void check(bool ok){if(!ok){puts("FAIL: glass reflected-light direction");std::exit(1);}}
int main(){
 const char* source=R"(
#include "pt_glass_lighting_direction.hlsli"
RWTexture2D<float4> output : register(u0);
[numthreads(1,1,1)] void main(uint3 id:SV_DispatchThreadID){
 float3 incident=id.x==0?float3(0,-3,4):id.x==1?float3(3,-4,0):float3(0,-6,8);
 float3 x=id.x==1?float3(0,0,-1):float3(1,0,0);
 float3 y=float3(0,1,0),z=id.x==1?float3(1,0,0):float3(0,0,1);
 float3 v=PTGlassLightView(incident,x,y,z);
 // A light on the reflected viewing direction has H=V, not camera-to-hit.
 output[id.xy]=float4(v,saturate(dot(float3(0,1,0),normalize(v+v))));
})";
 ComPtr<ID3D11Device> d;ComPtr<ID3D11DeviceContext> c;
 check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)));
 ComPtr<ID3DBlob> code,error;
 HRESULT hr=D3DCompile(source,strlen(source),"src/shaders/glass-direction-test.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
 if(error)fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);check(SUCCEEDED(hr));
 ComPtr<ID3D11ComputeShader> cs;check(SUCCEEDED(d->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&cs)));
 D3D11_TEXTURE2D_DESC desc{};desc.Width=3;desc.Height=1;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
 ComPtr<ID3D11Texture2D> target,readback;check(SUCCEEDED(d->CreateTexture2D(&desc,nullptr,&target)));
 ComPtr<ID3D11UnorderedAccessView> uav;check(SUCCEEDED(d->CreateUnorderedAccessView(target.Get(),nullptr,&uav)));
 desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(SUCCEEDED(d->CreateTexture2D(&desc,nullptr,&readback)));
 c->CSSetShader(cs.Get(),nullptr,0);ID3D11UnorderedAccessView* views[]={uav.Get()};c->CSSetUnorderedAccessViews(0,1,views,nullptr);c->Dispatch(3,1,1);
 ID3D11UnorderedAccessView* none[]={nullptr};c->CSSetUnorderedAccessViews(0,1,none,nullptr);c->CopyResource(readback.Get(),target.Get());
 D3D11_MAPPED_SUBRESOURCE mapped{};check(SUCCEEDED(c->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
 const float expected[12]={0,.6f,-.8f,.6f,0,.8f,.6f,.8f,0,.6f,-.8f,.6f};
 const float* actual=static_cast<const float*>(mapped.pData);for(unsigned i=0;i<12;++i)check(std::abs(actual[i]-expected[i])<.00001f);
 c->Unmap(readback.Get(),0);
 puts("PASS: production glass direction on WARP; reversed incident, rotated camera basis, segment-length invariance and light half-vector verified.");
}
