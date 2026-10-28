#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
using Microsoft::WRL::ComPtr;
static void check(bool ok){if(!ok){puts("FAIL: projection pair geometry");std::exit(1);}}
int main(){
 const char* source=R"(
#include "rt_spot_projection_probe.hlsli"
RWTexture2D<float4> output : register(u0);
[numthreads(1,1,1)] void main(uint3 id:SV_DispatchThreadID){
 float4 rows[4]={float4(1,0,0,0),float4(0,1,0,0),float4(0,0,1,1),float4(0,0,-1,0)};
 float3 apex;bool valid;
 if(id.x==0){valid=captureProjectionApex(rows,apex);output[id.xy]=float4(apex,valid);return;}
 if(id.x==1){rows[3]=float4(-2,-3,-1,-5);valid=captureProjectionApex(rows,apex);output[id.xy]=float4(apex,valid);return;}
 if(id.x==2){rows[0]=0;valid=captureProjectionApex(rows,apex);output[id.xy]=float4(apex,valid);return;}
 if(id.x==3){rows[0].x=asfloat(0x7fc00000);valid=captureProjectionApex(rows,apex);output[id.xy]=float4(apex,valid);return;}
 float3 direction;float maximum;
 float3 origin=float3(0,0,4),target=id.x==4?float3(2,0,0):float3(0,0,0);
 if(id.x==6)target=origin;
 float nearDistance=id.x==7?-1:1;
 valid=captureClippedSegment(origin,target,float3(0,0,0),float3(0,0,1),nearDistance,direction,maximum);
 output[id.xy]=valid?float4(direction,maximum):float4(0,0,0,0);
})";
 ComPtr<ID3D11Device> d;ComPtr<ID3D11DeviceContext> c;
 check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)));
 ComPtr<ID3DBlob> code,error;
 HRESULT hr=D3DCompile(source,strlen(source),"src/shaders/projection-pair-test.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
 if(error)fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);check(SUCCEEDED(hr));
 ComPtr<ID3D11ComputeShader> cs;check(SUCCEEDED(d->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&cs)));
 D3D11_TEXTURE2D_DESC desc{};desc.Width=8;desc.Height=1;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
 ComPtr<ID3D11Texture2D> target,readback;check(SUCCEEDED(d->CreateTexture2D(&desc,nullptr,&target)));
 ComPtr<ID3D11UnorderedAccessView> uav;check(SUCCEEDED(d->CreateUnorderedAccessView(target.Get(),nullptr,&uav)));
 desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(SUCCEEDED(d->CreateTexture2D(&desc,nullptr,&readback)));
 c->CSSetShader(cs.Get(),nullptr,0);ID3D11UnorderedAccessView* views[]={uav.Get()};c->CSSetUnorderedAccessViews(0,1,views,nullptr);c->Dispatch(8,1,1);
 ID3D11UnorderedAccessView* none[]={nullptr};c->CSSetUnorderedAccessViews(0,1,none,nullptr);c->CopyResource(readback.Get(),target.Get());
 D3D11_MAPPED_SUBRESOURCE mapped{};check(SUCCEEDED(c->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
 const float expected[32]={0,0,0,1, 2,3,5,1, 0,0,0,0, 0,0,0,0,
 .4472135955f,0,-.894427191f,3.35409877f, 0,0,-1,2.999997139f, 0,0,0,0, 0,0,0,0};
 const float* actual=static_cast<const float*>(mapped.pData);for(unsigned i=0;i<32;++i)check(std::abs(actual[i]-expected[i])<.00001f);
 c->Unmap(readback.Get(),0);
 puts("PASS: shared projection helper on WARP: finite apex, translated apex, singular/nonfinite rejection, independently clipped emitter/projection and invalid segment rejection.");
}
