#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cstdio>
int main(){
    ID3DBlob* code=nullptr; ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(L"src/shaders/fg_ui_alpha.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "GenerateUIAlpha","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors){std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr)||!code){if(code)code->Release();return 1;}
    ID3D11ShaderReflection* reflection=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(&reflection));
    if(FAILED(hr)||!reflection){code->Release();return 2;}
    UINT x=0,y=0,z=0;reflection->GetThreadGroupSize(&x,&y,&z);
    bool contract=x==8&&y==8&&z==1;
    D3D11_SHADER_INPUT_BIND_DESC finalColor{},hudless{},alpha{};
    contract=contract&&SUCCEEDED(reflection->GetResourceBindingDescByName("FinalColor",&finalColor))&&finalColor.Type==D3D_SIT_TEXTURE&&finalColor.BindPoint==0;
    contract=contract&&SUCCEEDED(reflection->GetResourceBindingDescByName("HudlessColor",&hudless))&&hudless.Type==D3D_SIT_TEXTURE&&hudless.BindPoint==1;
    contract=contract&&SUCCEEDED(reflection->GetResourceBindingDescByName("UIAlpha",&alpha))&&alpha.Type==D3D_SIT_UAV_RWTYPED&&alpha.BindPoint==0;
    reflection->Release();
    if(!contract){code->Release();std::fputs("FAIL: FG UI alpha shader ABI\n",stderr);return 3;}
    FILE* file=nullptr;if(fopen_s(&file,"build/fg_ui_alpha_compiled.h","wb")||!file){code->Release();return 4;}
    std::fprintf(file,"#pragma once\nstatic const unsigned char kFGUIAlphaShader[]={\n");
    const auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
    for(size_t i=0;i<code->GetBufferSize();++i)std::fprintf(file,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
    std::fprintf(file,"\n};\n");
    bool ok=!std::ferror(file);int closed=std::fclose(file);code->Release();
    if(!ok||closed)return 5;
    std::puts("PASS: FG UI-alpha shader compiled cs_5_0; t0/t1/u0 and 8x8x1 threads verified");
    return 0;
}
