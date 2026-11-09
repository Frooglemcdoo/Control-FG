#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cstdio>
int main() {
    ID3DBlob* code=nullptr; ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(L"src/shaders/rr_skin_mask.hlsl",nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,"GenerateSkinResponsivity","cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors){std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr)||!code){if(code)code->Release();return 1;}
    ID3D11ShaderReflection* reflection=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(&reflection));
    if(FAILED(hr)||!reflection){code->Release();return 2;}
    UINT x=0,y=0,z=0;reflection->GetThreadGroupSize(&x,&y,&z);
    bool contract=x==8&&y==8&&z==1;
    D3D11_SHADER_INPUT_BIND_DESC srv{},uav{};
    contract=contract&&SUCCEEDED(reflection->GetResourceBindingDescByName("CharacterCoverage",&srv))&&srv.Type==D3D_SIT_TEXTURE&&srv.BindPoint==0&&srv.BindCount==1;
    contract=contract&&SUCCEEDED(reflection->GetResourceBindingDescByName("ResponsivityMask",&uav))&&uav.Type==D3D_SIT_UAV_RWTYPED&&uav.BindPoint==0&&uav.BindCount==1;
    reflection->Release();
    if(!contract){code->Release();std::fputs("FAIL: skin responsivity shader ABI\n",stderr);return 3;}
    FILE* file=nullptr;if(fopen_s(&file,"build/rr_skin_mask_compiled.h","wb")||!file){code->Release();return 4;}
    std::fprintf(file,"#pragma once\nstatic const unsigned char kRRSkinMaskShader[]={\n");
    const auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
    for(size_t i=0;i<code->GetBufferSize();++i)std::fprintf(file,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
    std::fprintf(file,"\n};\n");
    const bool okay=!std::ferror(file);const int closed=std::fclose(file);code->Release();
    if(!okay||closed)return 5;
    std::puts("PASS: skin responsivity compute shader compiled cs_5_0; t0/u0 and 8x8x1 threads verified");
    return 0;
}
