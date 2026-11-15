#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <wrl/client.h>
#include <cstdio>
#include <initializer_list>
using Microsoft::WRL::ComPtr;
int main(){
 FILE* f=nullptr;if(fopen_s(&f,"build/gi_excess.generated.h","wb")||!f)return 1;
 fprintf(f,"#pragma once\n");
 for(const char* entry:{"Classify","Gather"}){
  ComPtr<ID3DBlob> code,error;
  HRESULT hr=D3DCompileFromFile(L"src/shaders/gi_excess_redistribution.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,entry,"cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
  if(error)fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);
  if(FAILED(hr)){fclose(f);return 2;}
  ComPtr<ID3D12ShaderReflection> reflect;if(FAILED(D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),IID_PPV_ARGS(&reflect)))){fclose(f);return 3;}
  const char* names[]={"GBuffer1","GBuffer2","LinearDepth","Coverage","RawGI","Donors"};const UINT slots[]={0,1,2,24,0,2};
  for(UINT i=0;i<6;++i){D3D12_SHADER_INPUT_BIND_DESC b{};
   if(FAILED(reflect->GetResourceBindingDescByName(names[i],&b))||b.BindPoint!=slots[i]||b.Space){fclose(f);return 4;}}
  UINT x,y,z;reflect->GetThreadGroupSize(&x,&y,&z);if(x!=8||y!=8||z!=1){fclose(f);return 5;}
  fprintf(f,"inline constexpr unsigned char kGIExcess%s[]={\n",entry);
  auto p=static_cast<const unsigned char*>(code->GetBufferPointer());for(SIZE_T i=0;i<code->GetBufferSize();++i)fprintf(f,"%u,",p[i]);fprintf(f,"\n};\n");
 }
 const bool okay=!ferror(f);return fclose(f)==0&&okay?0:6;
}
