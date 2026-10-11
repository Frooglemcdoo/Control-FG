#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>
#include <cstring>

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
        diagnostic==2u?L"src/shaders/pt_reflection_p2_composite.hlsl":
            L"src/shaders/pt_reflection_p2_native_composite.hlsl",
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
    if(diagnostic!=2u){
        ok=ok&&Binding(r,"g_sbMaterialDataPart3",D3D_SIT_STRUCTURED,4,0);
        ok=ok&&Binding(r,"g_sEnvironmentCube",D3D_SIT_TEXTURE,8,0);
    }
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
    if(diagnostic!=2u)
        ok=ok&&Binding(r,"g_sLinearWrap",D3D_SIT_SAMPLER,5,1);
    ok=ok&&Binding(r,"g_rwtReflectionTarget",D3D_SIT_UAV_RWTYPED,0,0);
    ok=ok&&Binding(r,"sys_constants",D3D_SIT_CBUFFER,0,0);
    if(diagnostic!=2u)
        ok=ok&&Binding(r,"env_general",D3D_SIT_CBUFFER,2,0);
    // Only normal mode uses the native intensity clamp; diagnostics no longer
    // reference b6 after replacing the unused material-count field.
    if(diagnostic!=2u)ok=ok&&Binding(r,"rtreflection",D3D_SIT_CBUFFER,6,0);
    if(diagnostic!=2u){
        ok=ok&&Binding(r,"g_ptPrivateHits",D3D_SIT_UAV_RWSTRUCTURED,0,99);
        const struct {const char* name;UINT slot;D3D_SHADER_INPUT_TYPE type;UINT stride;} required[]={
            {"g_sbMaterialDataPart1",3,D3D_SIT_STRUCTURED,8},
            {"g_tRaytracingLightCellPointLights",5,D3D_SIT_TEXTURE,0},
            {"g_tRaytracingLightCellSpotLights",6,D3D_SIT_TEXTURE,0},
            {"g_tPointLightShadowAtlas",9,D3D_SIT_TEXTURE,0},
            {"g_sbDeferredLightPointData",10,D3D_SIT_STRUCTURED,160},
            {"g_tSpotLightPCFShadowAtlas",11,D3D_SIT_TEXTURE,0},
            {"g_tSpotLightVSMShadowAtlas",12,D3D_SIT_TEXTURE,0},
            {"g_tSpotLightProjectionAtlas",13,D3D_SIT_TEXTURE,0},
            {"g_sbDeferredLightSpotData",14,D3D_SIT_STRUCTURED,320},
            {"g_tEnvBRDF",15,D3D_SIT_TEXTURE,0},
            {"g_sbActiveVolumeSamplingInfoArray",16,D3D_SIT_STRUCTURED,80}
        };
        for(const auto& requiredBinding:required){
            D3D12_SHADER_INPUT_BIND_DESC desc{};
            const bool present=SUCCEEDED(r->GetResourceBindingDescByName(requiredBinding.name,&desc));
            const bool valid=present&&desc.BindPoint==requiredBinding.slot&&desc.Space==0&&
                desc.Type==requiredBinding.type&&(!requiredBinding.stride||desc.NumSamples==requiredBinding.stride);
            std::printf("PT NATIVE SHADING ABI variant=%u resource=%s valid=%u\n",diagnostic,requiredBinding.name,unsigned(valid));
            ok=ok&&valid;
        }
        ok=ok&&Binding(r,"g_sLinearClamp",D3D_SIT_SAMPLER,6,1);
        ok=ok&&Binding(r,"g_sMaterialArrayGlobal",D3D_SIT_SAMPLER,19,1);
        ok=ok&&Binding(r,"g_tGIBrickTextures",D3D_SIT_TEXTURE,19,0,6);
        ok=ok&&Binding(r,"deferredlight_constants",D3D_SIT_CBUFFER,1,0);
        ok=ok&&Binding(r,"illuminationvolume",D3D_SIT_CBUFFER,3,0);
        ok=ok&&Binding(r,"ManualUpdateCB_ActiveVolumeTopLevelInfoArray",D3D_SIT_CBUFFER,4,0);
        ok=ok&&Variable(r,"sys_constants","ptCB0",0,1136);
    }
    // All variants must query the actual material SRV, never rely on the
    // unused native b6+12 constant. Check the compiled bytecode and reflection.
    D3D12_SHADER_INPUT_BIND_DESC materialBuffer{};
    const bool materialBufferOkay=SUCCEEDED(r->GetResourceBindingDescByName(
        "g_sbMaterialDataPart3",&materialBuffer))&&materialBuffer.BindPoint==4u&&
        materialBuffer.Space==0u&&materialBuffer.NumSamples==336u;
    auto* unusedVar=r->GetConstantBufferByName("rtreflection")->GetVariableByName("g_rtUnusedMaterialCount");
    D3D12_SHADER_VARIABLE_DESC unusedDesc{};
    const bool unusedCountOkay=FAILED(unusedVar->GetDesc(&unusedDesc))||
        (unusedDesc.uFlags&D3D_SVF_USED)==0u;
    ID3DBlob* assembly=nullptr;
    const bool hasBufferQuery=SUCCEEDED(D3DDisassemble(code->GetBufferPointer(),
        code->GetBufferSize(),0,nullptr,&assembly))&&assembly&&
        std::strstr(static_cast<const char*>(assembly->GetBufferPointer()),"bufinfo")!=nullptr;
    if(assembly)assembly->Release();
    std::printf("P2 MATERIAL BOUNDS variant=%u srv_stride336=%u native_count_unused=%u compiled_buffer_query=%u\n",
        diagnostic,unsigned(materialBufferOkay),unsigned(unusedCountOkay),unsigned(hasBufferQuery));
    ok=ok&&materialBufferOkay&&unusedCountOkay&&hasBufferQuery;
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
        // Only the two used native UINT light-count members are mandatory.
        // D3DCompile can strip unused sun/ambient symbols at optimization 3.
        const bool pointOffsetOkay=
            Variable(r,"deferredlight_constants","ptNativePointLightTotalCount",256,4);
        const bool spotOffsetOkay=
            Variable(r,"deferredlight_constants","ptNativeSpotLightTotalCount",260,4);
        const bool countOffsetOkay=pointOffsetOkay&&spotOffsetOkay;
        std::printf("PT LOCAL LIGHT ABI point_cell_t5=%u point_data_t10=%u point_stride160=%u spot_cell_t6=%u spot_data_t14=%u spot_stride320=%u deferred_b1=%u cb416=%u point_uint_at256=%u spot_uint_at260=%u\n",
            unsigned(cellOkay),unsigned(lightOkay),unsigned(strideOkay),
            unsigned(spotCellOkay),unsigned(spotLightOkay),unsigned(spotStrideOkay),
            unsigned(constantsOkay),unsigned(cbSizeOkay),
            unsigned(pointOffsetOkay),unsigned(spotOffsetOkay));
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
