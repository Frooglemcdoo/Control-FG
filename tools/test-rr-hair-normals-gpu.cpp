// Execute production guide HLSL on WARP and compare with its pre-filter path.
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
#include <cstdint>
using Microsoft::WRL::ComPtr;
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
static void hr(HRESULT value){check(SUCCEEDED(value),"D3D call");}
struct Pixel {float v[4];};
int main(){
 constexpr UINT W=32,H=32,N=W*H;
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
 hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
 std::ifstream file("src/shaders/rr_live_guides.hlsl");
 std::string source((std::istreambuf_iterator<char>(file)),{});
 const std::string call="normalView = HairNormalGuide(tid.xy, normalView);";
 const auto pos=source.find(call);check(pos!=std::string::npos,"filter call found");
 std::string baseline=source;baseline.replace(pos,call.size(),"normalView = normalView;");
 auto compile=[&](const std::string& text){ComPtr<ID3DBlob> blob,error;
  HRESULT result=D3DCompile(text.data(),text.size(),"src/shaders/rr_live_guides.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"GenerateLiveGuides","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&blob,&error);
  if(error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);hr(result);
  ComPtr<ID3D11ComputeShader> shader;hr(device->CreateComputeShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&shader));return shader;};
 std::string previous=source;
 const auto begin=previous.find("    // R12_BEGIN"),end=previous.find("    // R12_END");
 check(begin!=std::string::npos&&end!=std::string::npos,"R12 transition markers");
 previous.erase(begin,end-begin);
 auto original=compile(baseline),candidate=compile(source),r11=compile(previous);
 std::array<ComPtr<ID3D11Texture2D>,5> inputs;
 std::array<ComPtr<ID3D11ShaderResourceView>,5> srvs;
 std::vector<Pixel> g1(N),g2(N),albedo(N),env(N);
 for(UINT i=0;i<N;++i){UINT x=i%W,y=i/W;
  g1[i]={{(x%2?150.0f:110.0f)/255.0f,130.0f/255.0f,0.0f,0.0f}};
  g2[i]={{0.0f,0.1f,0.0f,0.0f}}; env[i]={{0.5f,0.1f,0.0f,0.0f}};
  // Tagged hair touches top/left texture bounds; untagged skin includes black.
  albedo[i]={{x%3?0.2f:0.0f,0.1f,0.05f,(x<16&&y<24)?-2.5f:0.0f}};
 }
 // A lone reversed hair normal within a coherent patch must be corrected.
 for(UINT y=2;y<=8;++y)for(UINT x=2;x<=8;++x)g1[y*W+x]={{0.5f,0.5f,0.0f,0.0f}};
 g1[5*W+5]={{1.0f,1.0f,0.0f,1.0f}};
 // An identical normal from a different material must retain its direction.
 g1[5*W+10]={{1.0f,1.0f,0.0f,1.0f}};g2[5*W+10].v[3]=1.0f/255.0f;
 // A thin patch whose 3x3 center majority is the weak, reversed card edge.
 // Stronger 5x5 neighbors must anchor the guide without touching adjacent skin.
 for(UINT y=26;y<=30;++y)for(UINT x=20;x<=24;++x){
  g1[y*W+x]={{0.5f,0.5f,0.0f,0.0f}};
  albedo[y*W+x]={{0.2f,0.1f,0.05f,-2.5f}};
 }
 for(UINT y=27;y<=29;++y)for(UINT x=21;x<=23;++x){
  g1[y*W+x]={{1.0f,1.0f,0.0f,1.0f}};
  albedo[y*W+x].v[3]=-2.01f;
 }
 // Conflicting same-material normals exercise the low-coherence path.
 for(UINT y=26;y<=30;++y)for(UINT x=26;x<=30;++x){
  g1[y*W+x]={{x%2?1.0f:0.0f,0.5f,0.0f,0.0f}};
  albedo[y*W+x]={{0.2f,0.1f,0.05f,-2.5f}};
 }
 auto texture=[&](UINT slot,const std::vector<Pixel>& values){D3D11_TEXTURE2D_DESC d{};
  d.Width=W;d.Height=H;d.MipLevels=1;d.ArraySize=1;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA init{values.data(),W*sizeof(Pixel),0};hr(device->CreateTexture2D(&d,&init,&inputs[slot]));hr(device->CreateShaderResourceView(inputs[slot].Get(),nullptr,&srvs[slot]));};
 texture(0,g1);texture(1,g2);texture(3,env);texture(4,albedo);
 const UINT material[4]={0x20202001u,0,0x20202001u,0};D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(material);bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=8;
 D3D11_SUBRESOURCE_DATA init{material,0,0};ComPtr<ID3D11Buffer> table;hr(device->CreateBuffer(&bd,&init,&table));hr(device->CreateShaderResourceView(table.Get(),nullptr,&srvs[2]));
 std::array<ComPtr<ID3D11Texture2D>,4> outputs,staging;
 std::array<ComPtr<ID3D11UnorderedAccessView>,4> uavs;
 for(UINT j=0;j<4;++j){D3D11_TEXTURE2D_DESC d{};d.Width=W;d.Height=H;d.MipLevels=1;d.ArraySize=1;d.Format=j==3?DXGI_FORMAT_R16_FLOAT:DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
  hr(device->CreateTexture2D(&d,nullptr,&outputs[j]));hr(device->CreateUnorderedAccessView(outputs[j].Get(),nullptr,&uavs[j]));d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;hr(device->CreateTexture2D(&d,nullptr,&staging[j]));}
 D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;ComPtr<ID3D11SamplerState> sampler;hr(device->CreateSamplerState(&sd,&sampler));
 auto run=[&](ID3D11ComputeShader* shader,UINT flags){UINT constants[4]={1,flags,W,H};D3D11_BUFFER_DESC d{};d.ByteWidth=16;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;D3D11_SUBRESOURCE_DATA data{constants,0,0};ComPtr<ID3D11Buffer> cb;hr(device->CreateBuffer(&d,&data,&cb));
  ID3D11ShaderResourceView* views[5];for(UINT j=0;j<5;++j)views[j]=srvs[j].Get();ID3D11UnorderedAccessView* targets[4];for(UINT j=0;j<4;++j)targets[j]=uavs[j].Get();
  context->CSSetShader(shader,nullptr,0);context->CSSetShaderResources(0,5,views);context->CSSetUnorderedAccessViews(0,4,targets,nullptr);auto* c=cb.Get();context->CSSetConstantBuffers(0,1,&c);auto* s=sampler.Get();context->CSSetSamplers(0,1,&s);context->Dispatch(2,2,1);
  ID3D11UnorderedAccessView* nulls[4]={};context->CSSetUnorderedAccessViews(0,4,nulls,nullptr);
  std::array<std::vector<Pixel>,4> result;
  for(UINT j=0;j<4;++j){context->CopyResource(staging[j].Get(),outputs[j].Get());D3D11_MAPPED_SUBRESOURCE mapped{};hr(context->Map(staging[j].Get(),0,D3D11_MAP_READ,0,&mapped));result[j].resize(N);
   for(UINT y=0;y<H;++y){const auto* row=static_cast<const char*>(mapped.pData)+y*mapped.RowPitch;
    if(j<3)std::memcpy(result[j].data()+y*W,row,W*sizeof(Pixel));
    else for(UINT x=0;x<W;++x){std::uint16_t h=0;std::memcpy(&h,row+x*2,2);const int e=(h>>10)&31;const float m=float(h&1023);float v=e?std::ldexp(1.0f+m/1024.0f,e-15):std::ldexp(m,-24);result[j][y*W+x].v[0]=(h&32768)?-v:v;}
   }context->Unmap(staging[j].Get(),0);}return result;};
 UINT changed=0,weakChanged=0;
 for(UINT flags : {0u,1u,3u,4u,5u,7u}){auto before=run(original.Get(),flags),after=run(candidate.Get(),flags),prior=run(r11.Get(),flags);
  if((flags&5u)==5u){
   check(!std::memcmp(&prior[0][28*W+22],&after[0][28*W+22],sizeof(Pixel)),"R11 coherent direction changed");
   for(UINT y=27;y<=29;++y)for(UINT x=27;x<=29;++x)
    if(std::memcmp(&prior[0][y*W+x],&after[0][y*W+x],12))++weakChanged;
   check(after[0][28*W+22].v[2]<-0.9f,"weak reversed card edge overrode coherent opaque patch");
   check(before[0][5*W+5].v[2]>0.5f,"outlier fixture not reversed");
   check(after[0][5*W+5].v[2]<-0.9f,"majority failed to recover isolated reversed hair normal");
   for(UINT c=0;c<3;++c)check(std::abs(before[0][5*W+10].v[c]-after[0][5*W+10].v[c])<0.00001f,"different-material singleton changed");
  }
  for(UINT i=0;i<N;++i){const bool hair=(albedo[i].v[3]>=-3.0f&&albedo[i].v[3]<=-2.0f)&&(flags&5u)==5u;
   for(UINT j=0;j<3;++j){if(!hair||j==1)check(!std::memcmp(&before[j][i],&after[j][i],sizeof(Pixel)),"untagged skin/fallback/diffuse changed");}
   const bool tagged=(albedo[i].v[3]>=-3.0f&&albedo[i].v[3]<=-2.0f)&&(flags&4u)!=0u;
   const float hint=after[3][i].v[0];
   check(std::isfinite(hint)&&hint>=-0.75f&&hint<=0.0f,"mask range");
   check(tagged?hint<0.0f:hint==0.0f,"hair mask leaked into skin or missing guide");
   if(hair){check(before[0][i].v[3]==after[0][i].v[3],"roughness changed");float len=0;for(UINT c=0;c<3;++c){float v=after[0][i].v[c];check(std::isfinite(v),"nonfinite normal");len+=v*v;}check(std::abs(len-1.0f)<0.0001f,"normal not unit length");if(std::memcmp(&before[0][i],&after[0][i],12))++changed;}
  }
 }
 check(weakChanged>0,"low-coherence transition was not exercised");
 check(changed>0,"hair normals unchanged");std::printf("PASS: production WARP shader changes hair normals (%u samples); isolated outlier corrected, different material rejected; skin, fallback, diffuse and roughness preserved; R16F hair hint negative and skin zero across six guide modes\n",changed);
}
