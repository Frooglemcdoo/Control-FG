#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>

static bool Binding(ID3D12ShaderReflection* r,const char* name,D3D_SHADER_INPUT_TYPE type,UINT slot,UINT space,UINT count=1) {
    D3D12_SHADER_INPUT_BIND_DESC d{};
    return r && SUCCEEDED(r->GetResourceBindingDescByName(name,&d)) &&
        d.Type==type && d.BindPoint==slot && d.BindCount==count && d.Space==space;
}
int main() {
    ID3DBlob* code=nullptr;
    ID3DBlob* errors=nullptr;
    const UINT flags=D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS;
    HRESULT hr=D3DCompileFromFile(
        L"src/shaders/pt_reflection_p2_diffuse_bridge.hlsl",
        nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main","cs_5_1",flags,0,&code,&errors);
    if(errors){
        std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);
        errors->Release();
    }
    if(FAILED(hr)||!code){if(code)code->Release();return 1;}

    ID3D12ShaderReflection* r=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),
        __uuidof(ID3D12ShaderReflection),reinterpret_cast<void**>(&r));
    if(FAILED(hr)||!r){code->Release();return 2;}

    UINT x=0,y=0,z=0;
    r->GetThreadGroupSize(&x,&y,&z);
    bool ok=x==8&&y==8&&z==1;
    ok=ok&&Binding(r,"g_tP2Reflection",D3D_SIT_TEXTURE,0,0);
    ok=ok&&Binding(r,"g_tGBuffer1",D3D_SIT_TEXTURE,1,0);
    ok=ok&&Binding(r,"g_rwDiffuseGI",D3D_SIT_UAV_RWTYPED,0,0);
    ok=ok&&Binding(r,"PTDiffuseBridgeConstants",D3D_SIT_CBUFFER,0,0);
    r->Release();
    if(!ok){code->Release();std::fputs("FAIL: diffuse bridge ABI mismatch\n",stderr);return 3;}

    FILE* file=nullptr;
    if(fopen_s(&file,"build/pt_reflection_p2_diffuse_bridge.dxbc","wb")||!file){
        code->Release();return 4;
    }
    const size_t written=std::fwrite(code->GetBufferPointer(),1,code->GetBufferSize(),file);
    const bool good=written==code->GetBufferSize()&&std::fclose(file)==0;
    std::printf("PASS: P2 diffuse bridge cs_5_1 bytes=%zu threads=8x8x1 bindings t0,t1/u0/b0\n",
        code->GetBufferSize());
    code->Release();
    return good?0:5;
}
