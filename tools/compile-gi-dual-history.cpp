#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>
int main(){
 ID3DBlob *code=nullptr,*error=nullptr;
 auto hr=D3DCompileFromFile(L"src/shaders/gi_dual_history.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
 if(error){fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);error->Release();}if(FAILED(hr))return 1;
 ID3D12ShaderReflection* r=nullptr;if(FAILED(D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),IID_PPV_ARGS(&r))))return 2;
 const char* names[]={"Depth","PreviousDepth","Velocity","Current","History","Output","Sys","Filtering","LinearClamp","FastHistory","MomentsHistory","FastOutput","MomentsOutput"};
 const UINT slots[]={0,1,2,3,4,0,0,1,6,5,6,1,2};
 for(unsigned i=0;i<13;++i){D3D12_SHADER_INPUT_BIND_DESC d{};if(FAILED(r->GetResourceBindingDescByName(names[i],&d))||d.BindPoint!=slots[i]||d.Space!=(i==8?1u:0u))return 3;}
 UINT x,y,z;r->GetThreadGroupSize(&x,&y,&z);if(x!=8||y!=8||z!=1)return 4;r->Release();
 FILE* f=nullptr;if(fopen_s(&f,"build/gi_dual_history.generated.h","wb")||!f)return 5;
 fprintf(f,"#pragma once\ninline constexpr unsigned char kGIDualHistory[]={\n");
 auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());for(size_t i=0;i<code->GetBufferSize();++i)fprintf(f,"%u,",bytes[i]);
 fprintf(f,"\n};\n");bool good=!ferror(f);good=fclose(f)==0&&good;code->Release();return good?0:6;
}
