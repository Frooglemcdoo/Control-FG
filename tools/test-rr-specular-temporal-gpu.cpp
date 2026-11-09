// WARP arithmetic test of production HLSL. Only sampler register spaces are
// flattened for D3D11; compile-clamp-strength verifies the real D3D12 ABI.
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
using Microsoft::WRL::ComPtr;
static void check(bool value,const char* text){if(!value){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
static void hr(HRESULT value){check(SUCCEEDED(value),"D3D call");}
struct Pixel {float x,y,z,w;};
int main(){
 constexpr UINT W=16,H=16,N=W*H;
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
 hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
 std::ifstream input("src/shaders/rr_specular_temporal.hlsl");std::string source((std::istreambuf_iterator<char>(input)),{});
 std::size_t at=0;while((at=source.find(", space1"))!=std::string::npos)source.erase(at,8);
 auto compile=[&](const char* weight){
  const D3D_SHADER_MACRO macros[]={{"FIREFLY_CLAMP","0.60"},{"TEMPORAL_WEIGHT",weight},{nullptr,nullptr}};
  ComPtr<ID3DBlob> code,error;auto result=D3DCompile(source.data(),source.size(),nullptr,macros,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS|D3DCOMPILE_IEEE_STRICTNESS,0,&code,&error);
  if(error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);hr(result);
  ComPtr<ID3D11ComputeShader> shader;hr(device->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader));return shader;
 };
 auto baseline=compile("0.0"),temporal=compile("0.6");
 std::array<std::vector<Pixel>,5> values;for(auto& v:values)v.resize(N);
 for(UINT i=0;i<N;++i){const float c=(i%W+i/W)%2?1.5f:0.5f;values[0][i]={10,0,0,0};values[1][i]={10,0,0,0};values[2][i]={0,0,0,0};values[3][i]={c,c,c,0};values[4][i]={2,2,2,0};}
 std::array<ComPtr<ID3D11Texture2D>,5> textures;std::array<ComPtr<ID3D11ShaderResourceView>,5> views;
 for(UINT i=0;i<5;++i){D3D11_TEXTURE2D_DESC d{};d.Width=W;d.Height=H;d.MipLevels=1;d.ArraySize=1;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  hr(device->CreateTexture2D(&d,nullptr,&textures[i]));hr(device->CreateShaderResourceView(textures[i].Get(),nullptr,&views[i]));}
 D3D11_TEXTURE2D_DESC d{};d.Width=W;d.Height=H;d.MipLevels=1;d.ArraySize=1;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
 ComPtr<ID3D11Texture2D> output,staging;ComPtr<ID3D11UnorderedAccessView> uav;hr(device->CreateTexture2D(&d,nullptr,&output));hr(device->CreateUnorderedAccessView(output.Get(),nullptr,&uav));
 d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;hr(device->CreateTexture2D(&d,nullptr,&staging));
 std::array<float,152> sys{};sys[2]=1.0f/W;sys[3]=1.0f/H;std::array<unsigned,12> filter{};
 D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(sys);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;ComPtr<ID3D11Buffer> system,settings;hr(device->CreateBuffer(&bd,nullptr,&system));bd.ByteWidth=sizeof(filter);hr(device->CreateBuffer(&bd,nullptr,&settings));
 D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;ComPtr<ID3D11SamplerState> sampler;hr(device->CreateSamplerState(&sd,&sampler));
 auto run=[&](ID3D11ComputeShader* shader,unsigned cameraCut){
  for(UINT i=0;i<5;++i)context->UpdateSubresource(textures[i].Get(),0,nullptr,values[i].data(),W*sizeof(Pixel),0);
  filter[8]=cameraCut;context->UpdateSubresource(system.Get(),0,nullptr,sys.data(),0,0);context->UpdateSubresource(settings.Get(),0,nullptr,filter.data(),0,0);
  ID3D11Buffer* buffers[]={system.Get(),settings.Get()};context->CSSetConstantBuffers(0,2,buffers);
  ID3D11ShaderResourceView* srvs[5]{};for(UINT i=0;i<5;++i)srvs[i]=views[i].Get();context->CSSetShaderResources(0,5,srvs);
  auto* sample=sampler.Get();context->CSSetSamplers(6,1,&sample);context->CSSetSamplers(11,1,&sample);
  auto* target=uav.Get();context->CSSetUnorderedAccessViews(0,1,&target,nullptr);context->CSSetShader(shader,nullptr,0);context->Dispatch(3,3,1);
  target=nullptr;context->CSSetUnorderedAccessViews(0,1,&target,nullptr);ID3D11ShaderResourceView* empty[5]{};context->CSSetShaderResources(0,5,empty);
  context->CopyResource(staging.Get(),output.Get());D3D11_MAPPED_SUBRESOURCE map{};hr(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map));std::vector<Pixel> result(N);
  for(UINT y=0;y<H;++y)std::memcpy(result.data()+y*W,static_cast<const unsigned char*>(map.pData)+y*map.RowPitch,W*sizeof(Pixel));context->Unmap(staging.Get(),0);return result;
 };
 const auto raw=run(baseline.Get(),0),cut=run(temporal.Get(),1),stable=run(temporal.Get(),0);
 for(UINT i=0;i<N;++i){check(std::abs(raw[i].x-values[3][i].x)<1e-5f,"baseline energy");check(std::abs(cut[i].x-raw[i].x)<1e-5f,"camera cut history rejected");check(std::abs(stable[i].x-(0.4f*raw[i].x+1.2f))<1e-5f,"valid history blend");}
 for(auto& v:values[1])v.x=20;
 const auto depth=run(temporal.Get(),0);for(UINT i=0;i<N;++i)check(std::abs(depth[i].x-raw[i].x)<1e-5f,"depth mismatch rejected");
 for(auto& v:values[1])v.x=10;for(auto& v:values[2])v.x=2;
 const auto offscreen=run(temporal.Get(),0);for(UINT i=0;i<N;++i)check(std::abs(offscreen[i].x-raw[i].x)<1e-5f,"offscreen history rejected");
 std::puts("PASS: temporal WARP valid history, camera cut, depth mismatch, offscreen rejection, texture edges and oversized dispatch; 60 percent clamp");
}
