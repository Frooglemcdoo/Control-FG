#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cstdio>
// Build-time compiler, using the installed Windows SDK compiler. No shader
// compilation occurs on the game's render thread or at runtime.
int main() {
    ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(L"src/shaders/rr_distance.hlsl",nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors) {std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr) || !code) {if(code) code->Release();return 1;}
    ID3D11ShaderReflection* reflection=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(&reflection));
    if(FAILED(hr) || !reflection) {code->Release();return 4;}
    UINT x=0,y=0,z=0;reflection->GetThreadGroupSize(&x,&y,&z);
    bool contract=x==8 && y==8 && z==1;
    const char* resources[]={"NativeMaterialId","NativePositionTexcoordY","NativeClipDepth","Distance","Status","Parameters"};
    const D3D_SHADER_INPUT_TYPE types[]={D3D_SIT_TEXTURE,D3D_SIT_TEXTURE,D3D_SIT_TEXTURE,D3D_SIT_UAV_RWTYPED,D3D_SIT_UAV_RWTYPED,D3D_SIT_CBUFFER};
    const UINT slots[]={0,1,2,0,1,0};
    for(UINT i=0;i<6;++i) {
        D3D11_SHADER_INPUT_BIND_DESC d{};
        contract=contract && SUCCEEDED(reflection->GetResourceBindingDescByName(resources[i],&d)) && d.Type==types[i] && d.BindPoint==slots[i] && d.BindCount==1;
    }
    auto* cb=reflection->GetConstantBufferByName("Parameters");
    const char* names[]={"WorldFromViewRow0","Width","ClipToViewColumn0","ClipToViewColumn3","NativeInvOutputRes"};
    const UINT offsets[]={0,48,64,112,128};
    for(UINT i=0;i<5;++i) {
        D3D11_SHADER_VARIABLE_DESC d{};
        contract=contract && cb && SUCCEEDED(cb->GetVariableByName(names[i])->GetDesc(&d)) && d.StartOffset==offsets[i];
    }
    reflection->Release();
    if(!contract) {code->Release();std::fputs("FAIL: distance shader binding/constant ABI\n",stderr);return 5;}
    FILE* file=nullptr;
    if(fopen_s(&file,"build/rr_distance_compiled.h","wb") || !file) {code->Release();return 2;}
    std::fprintf(file,"#pragma once\nstatic const unsigned char kRRDistanceShader[]={\n");
    const auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
    for(size_t i=0;i<code->GetBufferSize();++i) std::fprintf(file,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
    std::fprintf(file,"\n};\n");
    const bool okay=!std::ferror(file);const int closed=std::fclose(file);code->Release();
    if(!okay || closed) return 3;
    std::puts("PASS: distance compute shader compiled cs_5_0; reflected bindings, constant offsets and 8x8x1 threads verified");return 0;
}
