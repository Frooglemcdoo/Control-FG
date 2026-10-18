#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>
static bool binding(ID3D12ShaderReflection* r,const char* name,D3D_SHADER_INPUT_TYPE type,UINT slot,UINT space=0){D3D12_SHADER_INPUT_BIND_DESC d{};return SUCCEEDED(r->GetResourceBindingDescByName(name,&d))&&d.Type==type&&d.BindPoint==slot&&d.Space==space;}
int main(){
 FILE* f=nullptr;if(fopen_s(&f,"build/rr_gi_shaders.h","wb")||!f)return 1;
 std::fputs("#pragma once\n",f);
 for(unsigned kind=0;kind<3;++kind){
  D3D_SHADER_MACRO macros[]={{"TEMPORAL_WEIGHT",kind==0?"0.35":"0.0"},{kind==1?"AXIS_X":"AXIS_Y","1"},{nullptr,nullptr}};
  ID3DBlob* code=nullptr;ID3DBlob* error=nullptr;
  HRESULT hr=D3DCompileFromFile(kind==0?L"src/shaders/rr_gi_temporal.hlsl":L"src/shaders/rr_gi_spatial.hlsl",macros,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
  if(error){std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);error->Release();}if(FAILED(hr))return 2;
  ID3D12ShaderReflection* r=nullptr;if(FAILED(D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),IID_PPV_ARGS(&r))))return 3;
  UINT x,y,z;r->GetThreadGroupSize(&x,&y,&z);
  bool okay=x==8&&y==8&&z==1&&binding(r,"target",D3D_SIT_UAV_RWTYPED,0)&&binding(r,"source",D3D_SIT_TEXTURE,kind==0?3:5);
  if(kind==0)okay=okay&&binding(r,"history_tex",D3D_SIT_TEXTURE,4)&&binding(r,"velocity_tex",D3D_SIT_TEXTURE,2)&&binding(r,"linear_clamp",D3D_SIT_SAMPLER,6,1)&&binding(r,"linear_mip_clamp",D3D_SIT_SAMPLER,11,1);
  else {D3D12_SHADER_INPUT_BIND_DESC d{};okay=okay&&FAILED(r->GetResourceBindingDescByName("history_tex",&d))&&binding(r,"hit_info",D3D_SIT_TEXTURE,7);}
  r->Release();if(!okay)return 4;
  std::fprintf(f,"inline const unsigned char kRRGI%u[]={\n",kind);const auto* b=static_cast<const unsigned char*>(code->GetBufferPointer());
  for(size_t i=0;i<code->GetBufferSize();++i)std::fprintf(f,"0x%02x,%s",b[i],i%16==15?"\n":"");std::fputs("\n};\n",f);code->Release();
 }
 std::fputs("struct RRGIBytes{const void* data;size_t size;};\ninline const RRGIBytes kRRGIShaders[]={{kRRGI0,sizeof(kRRGI0)},{kRRGI1,sizeof(kRRGI1)},{kRRGI2,sizeof(kRRGI2)}};\n",f);
 return std::fclose(f)==0?0:5;
}
