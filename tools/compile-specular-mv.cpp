#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cstdio>
int main() {
    ID3DBlob* code=nullptr; ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(L"src/shaders/rr_specular_mv.hlsl",nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors){std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr)||!code){if(code)code->Release();return 1;}
    ID3D11ShaderReflection* r=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(&r));
    if(FAILED(hr)||!r){code->Release();return 2;}
    UINT x=0,y=0,z=0;r->GetThreadGroupSize(&x,&y,&z);
    bool ok=x==16&&y==16&&z==1;
    const char* names[]={"GBuffer1","MatId","Depth","GameMV","HitPos","SpecMV","Debug","Params"};
    const D3D_SHADER_INPUT_TYPE types[]={D3D_SIT_TEXTURE,D3D_SIT_TEXTURE,D3D_SIT_TEXTURE,D3D_SIT_TEXTURE,D3D_SIT_TEXTURE,D3D_SIT_UAV_RWTYPED,D3D_SIT_UAV_RWTYPED,D3D_SIT_CBUFFER};
    const UINT slots[]={0,1,2,3,4,0,1,0};
    for(UINT i=0;i<8;++i){D3D11_SHADER_INPUT_BIND_DESC d{};ok=ok&&SUCCEEDED(r->GetResourceBindingDescByName(names[i],&d))&&d.Type==types[i]&&d.BindPoint==slots[i]&&d.BindCount==1;}
    auto* cb=r->GetConstantBufferByName("Params");D3D11_SHADER_BUFFER_DESC bd{};
    ok=ok&&cb&&SUCCEEDED(cb->GetDesc(&bd))&&bd.Size==224;
    struct Var {const char* name;UINT offset;UINT size;};
    const Var vars[]={{"ClipToView",0,64},{"ViewToClip",64,64},{"ClipToPrevClip",128,64},{"Width",192,4},{"Height",196,4},{"MvOutScaleX",200,4},{"MvOutScaleY",204,4},{"Layers",208,4}};
    for(const auto& v:vars){D3D11_SHADER_VARIABLE_DESC d{};ok=ok&&cb&&SUCCEEDED(cb->GetVariableByName(v.name)->GetDesc(&d))&&d.StartOffset==v.offset&&d.Size==v.size;}
    r->Release();
    if(!ok){code->Release();std::fputs("FAIL: GI27 specular MV shader ABI\n",stderr);return 3;}
    FILE* file=nullptr;if(fopen_s(&file,"build/rr_specular_mv_compiled.h","wb")||!file){code->Release();return 4;}
    std::fprintf(file,"#pragma once\nstatic const unsigned char kRRSpecularMvShader[]={\n");
    auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
    for(size_t i=0;i<code->GetBufferSize();++i)std::fprintf(file,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
    std::fprintf(file,"\n};\n");
    const bool good=!std::ferror(file);const int closed=std::fclose(file);code->Release();
    if(!good||closed)return 5;
    std::puts("PASS GI27 specular MV shader: t0..t4/u0..u1/b0, 16x16x1, 53 DWORD constants");
    return 0;
}
