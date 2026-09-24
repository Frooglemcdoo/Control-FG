#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cstdio>
// Build-time compiler, using the installed Windows SDK compiler. No shader
// compilation occurs on the game's render thread or at runtime.
int main() {
    ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(L"src/shaders/rr_reflectance_capture.hlsl",nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,"GenerateReflectanceCapture","cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors) {std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr) || !code) {if(code) code->Release();return 1;}
    ID3D11ShaderReflection* reflection=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(&reflection));
    if(FAILED(hr) || !reflection) {code->Release();return 4;}
    UINT x=0,y=0,z=0;reflection->GetThreadGroupSize(&x,&y,&z);
    D3D11_SHADER_INPUT_BIND_DESC input{},output{},layout{};
    bool contract=x==8 && y==8 && z==1 &&
        SUCCEEDED(reflection->GetResourceBindingDescByName("Captured",&input)) && input.Type==D3D_SIT_BYTEADDRESS && input.BindPoint==0 &&
        SUCCEEDED(reflection->GetResourceBindingDescByName("Result",&output)) && output.Type==D3D_SIT_UAV_RWBYTEADDRESS && output.BindPoint==0 &&
        SUCCEEDED(reflection->GetResourceBindingDescByName("Layout",&layout)) && layout.Type==D3D_SIT_CBUFFER && layout.BindPoint==0;
    auto* cb=reflection->GetConstantBufferByName("Layout");
    const char* names[]={"Width","Height","Records","ProjectionX","ProjectionY"};
    const UINT offsets[]={0,4,8,16,20};
    for(UINT i=0;i<5;++i) {
        D3D11_SHADER_VARIABLE_DESC d{};
        contract=contract && cb && SUCCEEDED(cb->GetVariableByName(names[i])->GetDesc(&d)) && d.StartOffset==offsets[i] && d.Size==4;
    }
    reflection->Release();
    if(!contract) {code->Release();std::fputs("FAIL: reflectance shader binding/constant ABI\n",stderr);return 5;}
    FILE* file=nullptr;
    if(fopen_s(&file,"build/rr_reflectance_compiled.h","wb") || !file) {code->Release();return 2;}
    std::fprintf(file,"#pragma once\nstatic const unsigned char kRRReflectanceShader[]={\n");
    const auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
    for(size_t i=0;i<code->GetBufferSize();++i) std::fprintf(file,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
    std::fprintf(file,"\n};\n");
    const bool okay=!std::ferror(file);const int closed=std::fclose(file);code->Release();
    if(!okay || closed) return 3;
    std::puts("PASS: reflectance compute shader compiled cs_5_0; reflected bindings, constant offsets and 8x8x1 threads verified");return 0;
}
