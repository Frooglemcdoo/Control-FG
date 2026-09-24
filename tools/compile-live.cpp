#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cstdio>
// Build-time compiler, using the installed Windows SDK compiler. No shader
// compilation occurs on the game's render thread or at runtime.
int main() {
    ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(L"src/shaders/rr_live_guides.hlsl",nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,"GenerateLiveGuides","cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors) {std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr) || !code) {if(code) code->Release();return 1;}
    ID3D11ShaderReflection* reflection=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(&reflection));
    if(FAILED(hr) || !reflection) {code->Release();return 4;}
    UINT x=0,y=0,z=0;reflection->GetThreadGroupSize(&x,&y,&z);
    bool contract=x==16 && y==16 && z==1;
    const char* resources[]={"GBuffer1","GBuffer2","MaterialDataPart1","EnvBRDF","LinearClamp","NormalRoughness","DiffuseGuide","SpecularGuide","GuideConstants"};
    const D3D_SHADER_INPUT_TYPE types[]={D3D_SIT_TEXTURE,D3D_SIT_TEXTURE,D3D_SIT_STRUCTURED,D3D_SIT_TEXTURE,D3D_SIT_SAMPLER,D3D_SIT_UAV_RWTYPED,D3D_SIT_UAV_RWTYPED,D3D_SIT_UAV_RWTYPED,D3D_SIT_CBUFFER};
    const UINT slots[]={0,1,2,3,0,0,1,2,0};
    for(UINT i=0;i<9;++i) {
        D3D11_SHADER_INPUT_BIND_DESC d{};
        contract=contract && SUCCEEDED(reflection->GetResourceBindingDescByName(resources[i],&d)) && d.Type==types[i] && d.BindPoint==slots[i] && d.BindCount==1;
    }
    auto* cb=reflection->GetConstantBufferByName("GuideConstants");
    const char* names[]={"InvertSmoothness","GuideFlags","Width","Height"};
    const UINT offsets[]={0,4,8,12};
    for(UINT i=0;i<4;++i) {
        D3D11_SHADER_VARIABLE_DESC d{};
        contract=contract && cb && SUCCEEDED(cb->GetVariableByName(names[i])->GetDesc(&d)) && d.StartOffset==offsets[i] && d.Size==4;
    }
    reflection->Release();
    if(!contract) {code->Release();std::fputs("FAIL: r21r retained GBuffer guide shader binding/constant ABI\n",stderr);return 5;}
    FILE* file=nullptr;
    if(fopen_s(&file,"build/rr_live_compiled.h","wb") || !file) {code->Release();return 2;}
    std::fprintf(file,"#pragma once\nstatic const unsigned char kRRLiveShader[]={\n");
    const auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
    for(size_t i=0;i<code->GetBufferSize();++i) std::fprintf(file,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
    std::fprintf(file,"\n};\n");
    const bool okay=!std::ferror(file);const int closed=std::fclose(file);code->Release();
    if(!okay || closed) return 3;
    std::puts("PASS: r21r retained GBuffer guide compute compiled cs_5_0; t0/t1/t2/t3, s0, u0/u1/u2, b0 and 16x16x1 threads verified");return 0;
}
