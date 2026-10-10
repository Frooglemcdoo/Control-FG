#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>

static bool Binding(ID3D12ShaderReflection* r,const char* name,D3D_SHADER_INPUT_TYPE type,UINT slot,UINT space,UINT count=1) {
    D3D12_SHADER_INPUT_BIND_DESC d{};
    return r && SUCCEEDED(r->GetResourceBindingDescByName(name,&d)) &&
        d.Type==type && d.BindPoint==slot && d.BindCount==count && d.Space==space;
}
static bool BindingAnyCount(ID3D12ShaderReflection* r,const char* name,D3D_SHADER_INPUT_TYPE type,UINT slot,UINT space) {
    D3D12_SHADER_INPUT_BIND_DESC d{};
    return r && SUCCEEDED(r->GetResourceBindingDescByName(name,&d)) &&
        d.Type==type && d.BindPoint==slot && d.BindCount>=1 && d.Space==space;
}
static bool Variable(ID3D12ShaderReflection* r,const char* cbName,const char* varName,UINT offset,UINT size) {
    auto* cb=r?r->GetConstantBufferByName(cbName):nullptr;
    auto* v=cb?cb->GetVariableByName(varName):nullptr;
    D3D12_SHADER_VARIABLE_DESC d{};
    return v&&SUCCEEDED(v->GetDesc(&d))&&d.StartOffset==offset&&d.Size==size;
}
int main() {
    ID3DBlob* code=nullptr;
    ID3DBlob* errors=nullptr;
    const UINT flags=D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS|
        D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
    HRESULT hr=D3DCompileFromFile(
        L"src/shaders/pt_reflection_p2_composite.hlsl",
        nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main","cs_5_1",flags,0,&code,&errors);
    if(errors){
        std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);
        errors->Release();
    }
    if(FAILED(hr)||!code){
        if(code)code->Release();
        return 1;
    }

    ID3D12ShaderReflection* r=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),
        __uuidof(ID3D12ShaderReflection),reinterpret_cast<void**>(&r));
    if(FAILED(hr)||!r){code->Release();return 2;}

    UINT x=0,y=0,z=0;
    r->GetThreadGroupSize(&x,&y,&z);
    bool ok=x==8&&y==16&&z==1;
    ok=ok&&Binding(r,"g_sbMaterialDataPart3",D3D_SIT_STRUCTURED,4,0);
    ok=ok&&Binding(r,"g_sEnvironmentCube",D3D_SIT_TEXTURE,8,0);
    D3D12_SHADER_INPUT_BIND_DESC materialTextures{};
    const bool materialTexturesPresent=
        SUCCEEDED(r->GetResourceBindingDescByName("g_sMaterialTextureArray",&materialTextures));
    if(materialTexturesPresent)
        std::printf("P2 material texture array type=%u slot=%u count=%u space=%u\n",
            unsigned(materialTextures.Type),materialTextures.BindPoint,
            materialTextures.BindCount,materialTextures.Space);
    ok=ok&&Binding(r,"g_tMaterialId",D3D_SIT_TEXTURE,26,0);
    ok=ok&&Binding(r,"g_tNormal_TexcoordX",D3D_SIT_TEXTURE,27,0);
    ok=ok&&Binding(r,"g_tPosition_TexcoordY",D3D_SIT_TEXTURE,28,0);
    ok=ok&&Binding(r,"g_sLinearWrap",D3D_SIT_SAMPLER,5,1);
    ok=ok&&Binding(r,"g_rwtReflectionTarget",D3D_SIT_UAV_RWTYPED,0,0);
    ok=ok&&Binding(r,"sys_constants",D3D_SIT_CBUFFER,0,0);
    ok=ok&&Binding(r,"env_general",D3D_SIT_CBUFFER,2,0);
    ok=ok&&Binding(r,"rtreflection",D3D_SIT_CBUFFER,6,0);
    ok=ok&&Variable(r,"sys_constants","g_mViewToWorld",96,64);
    ok=ok&&Variable(r,"env_general","g_fEnvReflectionMipCount",4,4);
    ok=ok&&Variable(r,"rtreflection","g_uMaterialIDCount",12,4);
    r->Release();
    if(!ok){
        code->Release();
        std::fputs("FAIL: P2 composite ABI mismatch\n",stderr);
        return 3;
    }

    FILE* file=nullptr;
    if(fopen_s(&file,"build/pt_reflection_p2_composite.dxbc","wb")||!file){
        code->Release();
        return 4;
    }
    const size_t written=std::fwrite(code->GetBufferPointer(),1,code->GetBufferSize(),file);
    const bool good=written==code->GetBufferSize()&&std::fclose(file)==0;
    std::printf("PASS: P2 composite cs_5_1 bytes=%zu threads=8x16x1 native bindings t4,t8,t26,t27,t28/s5(space1)/u0/b0,b2,b6 material_texture_array_present=%u\n",
        code->GetBufferSize(),unsigned(materialTexturesPresent));
    code->Release();
    return good?0:5;
}
