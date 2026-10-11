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
    for(unsigned diagnostic=0;diagnostic<=2;diagnostic++){
    const char* diagnosticValue=diagnostic==2u?"2":"1";
    const D3D_SHADER_MACRO verifyMacros[]={{"PT_REFLECTION_VERIFY_VARIANT",diagnosticValue},{nullptr,nullptr}};
    ID3DBlob* code=nullptr;
    ID3DBlob* errors=nullptr;
    const UINT flags=D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS|
        D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
    HRESULT hr=D3DCompileFromFile(
        L"src/shaders/pt_reflection_p2_composite.hlsl",
        diagnostic?verifyMacros:nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,
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
    if(diagnostic==2u){
        // The native shader RDEF identifies these exact bindings. Verify
        // no untested descriptor/root ABI gets into the game package.
        const bool cellOkay=Binding(r,"ptNativePointCellIds",D3D_SIT_TEXTURE,5,0);
        const bool lightOkay=Binding(r,"ptNativePointLights",D3D_SIT_STRUCTURED,10,0);
        const bool spotCellOkay=Binding(r,"ptNativeSpotCellIds",D3D_SIT_TEXTURE,6,0);
        const bool spotLightOkay=Binding(r,"ptNativeSpotLights",D3D_SIT_STRUCTURED,14,0);
        const bool constantsOkay=Binding(r,"deferredlight_constants",D3D_SIT_CBUFFER,1,0);
        D3D12_SHADER_INPUT_BIND_DESC light{};
        const bool reflection=SUCCEEDED(r->GetResourceBindingDescByName("ptNativePointLights",&light));
        const bool strideOkay=reflection&&light.NumSamples==160u;
        D3D12_SHADER_INPUT_BIND_DESC spot{};
        const bool spotReflection=SUCCEEDED(r->GetResourceBindingDescByName("ptNativeSpotLights",&spot));
        const bool spotStrideOkay=spotReflection&&spot.NumSamples==320u;
        auto* cb=r->GetConstantBufferByName("deferredlight_constants");
        D3D12_SHADER_BUFFER_DESC cbdesc{};
        const bool cbSizeOkay=cb&&SUCCEEDED(cb->GetDesc(&cbdesc))&&cbdesc.Size==416u;
        const bool countOffsetOkay=Variable(r,"deferredlight_constants","ptNativeDeferredConstants",0,416);
        std::printf("PT LOCAL LIGHT ABI point_cell_t5=%u point_data_t10=%u point_stride160=%u spot_cell_t6=%u spot_data_t14=%u spot_stride320=%u deferred_b1=%u cb416=%u count_offset256_260=%u\n",
            unsigned(cellOkay),unsigned(lightOkay),unsigned(strideOkay),
            unsigned(spotCellOkay),unsigned(spotLightOkay),unsigned(spotStrideOkay),
            unsigned(constantsOkay),unsigned(cbSizeOkay),unsigned(countOffsetOkay));
        ok=ok&&cellOkay&&lightOkay&&strideOkay&&spotCellOkay&&spotLightOkay&&
            spotStrideOkay&&constantsOkay&&cbSizeOkay&&countOffsetOkay;
    }
    r->Release();
    if(!ok){
        code->Release();
        std::fputs("FAIL: P2 composite ABI mismatch\n",stderr);
        return 3;
    }

    FILE* file=nullptr;
    const char* output=diagnostic==2u?
        "build/pt_reflection_p2_local_light_probe.dxbc":
        (diagnostic==1u?"build/pt_reflection_p2_verify.dxbc":
                        "build/pt_reflection_p2_composite.dxbc");
    if(fopen_s(&file,output,"wb")||!file){
        code->Release();
        return 4;
    }
    const size_t written=std::fwrite(code->GetBufferPointer(),1,code->GetBufferSize(),file);
    const bool good=written==code->GetBufferSize()&&std::fclose(file)==0;
    std::printf("PASS: P2 composite verify=%u output=%s bytes=%zu threads=8x16x1 native bindings t4,t8,t26,t27,t28/s5(space1)/u0/b0,b2,b6 material_texture_array_present=%u\n",
        diagnostic,output,code->GetBufferSize(),unsigned(materialTexturesPresent));
    code->Release();
    if(!good)return 5;
    }
    return 0;
}
