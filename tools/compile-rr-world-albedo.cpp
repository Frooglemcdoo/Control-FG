#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>
#include <cstring>
int main(){
 ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
 HRESULT hr=D3DCompileFromFile(L"src/shaders/rr_world_albedo.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,
  "WorldAlbedo","ps_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS|D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES,0,&code,&errors);
 if(errors){fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
 if(FAILED(hr)||!code)return 1;
 ID3D12ShaderReflection* reflection=nullptr;
 hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D12ShaderReflection),reinterpret_cast<void**>(&reflection));
 if(FAILED(hr)||!reflection)return 2;
 D3D12_SHADER_DESC desc{};reflection->GetDesc(&desc);bool good=desc.BoundResources==4&&desc.OutputParameters==1&&desc.TextureNormalInstructions==1;
 const char* names[]{"g_sbMaterialDataPart3","g_sMaterialTextureArray","g_sMaterialArrayGlobal","mid_general"};
 const UINT slots[]{0,0,19,0},spaces[]{0,1,1,0};
 const D3D_SHADER_INPUT_TYPE types[]{D3D_SIT_STRUCTURED,D3D_SIT_TEXTURE,D3D_SIT_SAMPLER,D3D_SIT_CBUFFER};
 for(unsigned i=0;i<4;++i){D3D12_SHADER_INPUT_BIND_DESC b{};good=good&&SUCCEEDED(reflection->GetResourceBindingDescByName(names[i],&b))&&b.BindPoint==slots[i]&&b.Space==spaces[i]&&b.Type==types[i];}
 bool uv=false;
 for(UINT i=0;i<desc.InputParameters;++i){D3D12_SIGNATURE_PARAMETER_DESC s{};reflection->GetInputParameterDesc(i,&s);
  if(!strcmp(s.SemanticName,"TEXCOORD")&&s.SemanticIndex==1)uv=s.Register==4&&s.Mask==3;}
 D3D12_SIGNATURE_PARAMETER_DESC output{};reflection->GetOutputParameterDesc(0,&output);
 good=good&&uv&&output.Register==0&&output.Mask==15;
 reflection->Release();if(!good){puts("FAIL world replay binding/input signature parity");return 3;}
 FILE* f=nullptr;if(fopen_s(&f,"build/rr_world_albedo_compiled.h","wb")||!f)return 4;
 fprintf(f,"#pragma once\nstatic const unsigned char kRRWorldAlbedoShader[]={\n");
 const auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
 for(size_t i=0;i<code->GetBufferSize();++i)fprintf(f,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
 fprintf(f,"\n};\n");bool okay=!ferror(f);okay=fclose(f)==0&&okay;
 ID3DBlob* assembly=nullptr;if(SUCCEEDED(D3DDisassemble(code->GetBufferPointer(),code->GetBufferSize(),0,nullptr,&assembly))){
  FILE* a=nullptr;if(!fopen_s(&a,"build/rr_world_albedo.asm","wb")&&a){fwrite(assembly->GetBufferPointer(),1,assembly->GetBufferSize(),a);fclose(a);}assembly->Release();}
 code->Release();puts("PASS world base albedo shader SM5.1 bindings and TEXCOORD1 register 4 parity");return okay?0:5;
}
