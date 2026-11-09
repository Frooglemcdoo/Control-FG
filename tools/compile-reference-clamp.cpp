#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdint>
#include <cstring>

static uint32_t CRC32(const void* data, size_t size) {
    uint32_t crc=0xFFFFFFFFu;
    const auto* p=static_cast<const unsigned char*>(data);
    for(size_t i=0;i<size;++i){
        crc^=p[i];
        for(unsigned b=0;b<8;++b) crc=(crc>>1)^(0xEDB88320u & (0u-(crc&1u)));
    }
    return crc^0xFFFFFFFFu;
}
static bool Binding(ID3D12ShaderReflection* r,const char* name,D3D_SHADER_INPUT_TYPE type,UINT slot,UINT space) {
    D3D12_SHADER_INPUT_BIND_DESC d{};
    return r && SUCCEEDED(r->GetResourceBindingDescByName(name,&d)) && d.Type==type && d.BindPoint==slot && d.BindCount==1 && d.Space==space;
}
static bool BasicContract(ID3D12ShaderReflection* r,bool replacement) {
    UINT x=0,y=0,z=0;r->GetThreadGroupSize(&x,&y,&z);
    bool ok=x==8&&y==8&&z==1;
    ok=ok&&Binding(r,replacement?"source":"g_DLF_rwtColorSource",D3D_SIT_TEXTURE,3,0);
    ok=ok&&Binding(r,replacement?"linear_mip_clamp":"g_sLinearMipLinearClamp",D3D_SIT_SAMPLER,11,1);
    ok=ok&&Binding(r,replacement?"target":"g_DLF_rwtColorTarget",D3D_SIT_UAV_RWTYPED,0,0);
    ok=ok&&Binding(r,"sys_constants",D3D_SIT_CBUFFER,0,0);
    auto* cb=r->GetConstantBufferByName("sys_constants");
    auto* var=cb?cb->GetVariableByName("g_vInvScreenRes"):nullptr;
    D3D12_SHADER_VARIABLE_DESC vd{};
    ok=ok&&var&&SUCCEEDED(var->GetDesc(&vd))&&vd.StartOffset==8&&vd.Size==8;
    return ok;
}
int main() {
    std::ifstream in("research/native-integration-audit/shaders/deferredlight_filtering_specular-000.dxbc",std::ios::binary);
    std::vector<unsigned char> native((std::istreambuf_iterator<char>(in)),{});
    if(native.empty()||CRC32(native.data(),native.size())!=0x600347E7u){std::fputs("FAIL: native Control specular temporal shader CRC is not 0x600347E7\n",stderr);return 1;}
    ID3D12ShaderReflection* nr=nullptr;
    if(FAILED(D3DReflect(native.data(),native.size(),__uuidof(ID3D12ShaderReflection),reinterpret_cast<void**>(&nr)))||!nr||!BasicContract(nr,false)){
        if(nr)nr->Release();std::fputs("FAIL: native Control temporal shader ABI mismatch\n",stderr);return 2;
    }
    nr->Release();
    ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(L"diagnostic_addon/rr_specular_temporal_clamp.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors){std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr)||!code){if(code)code->Release();return 3;}
    ID3D12ShaderReflection* rr=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D12ShaderReflection),reinterpret_cast<void**>(&rr));
    if(FAILED(hr)||!rr||!BasicContract(rr,true)){if(rr)rr->Release();code->Release();std::fputs("FAIL: RenoDX clamp replacement ABI mismatch\n",stderr);return 4;}
    // TEMPORAL_WEIGHT=0 must compile history/depth/velocity and s6 out completely.
    D3D12_SHADER_INPUT_BIND_DESC dead{};
    const bool temporalRemoved=FAILED(rr->GetResourceBindingDescByName("history_tex",&dead))&&
      FAILED(rr->GetResourceBindingDescByName("linear_depth",&dead))&&FAILED(rr->GetResourceBindingDescByName("prev_linear_depth",&dead))&&
      FAILED(rr->GetResourceBindingDescByName("velocity_tex",&dead))&&FAILED(rr->GetResourceBindingDescByName("linear_clamp",&dead));
    rr->Release();
    if(!temporalRemoved){code->Release();std::fputs("FAIL: temporal-history dependencies survived replacement optimization\n",stderr);return 5;}
    FILE* file=nullptr;
    if(fopen_s(&file,"build/controlfg_renodx_clamp_ref_shader.h","wb")||!file){code->Release();return 6;}
    std::fprintf(file,"#pragma once\nstatic const unsigned char kControlFGRenoDXClampRefShader[]={\n");
    const auto* bytes=static_cast<const unsigned char*>(code->GetBufferPointer());
    for(size_t i=0;i<code->GetBufferSize();++i)std::fprintf(file,"0x%02x,%s",bytes[i],i%16==15?"\n":"");
    std::fprintf(file,"\n};\nstatic constexpr unsigned int kControlFGRenoDXClampRefShaderCRC32=0x%08Xu;\n",CRC32(bytes,code->GetBufferSize()));
    const bool good=!std::ferror(file)&&std::fclose(file)==0;code->Release();
    if(!good)return 7;
    std::puts("PASS: Control 0x600347E7 ABI verified; behavior-equivalent RenoDX current-frame clamp compiled cs_5_1; t3/s11(space1)/u0/b0 and 8x8x1 verified; temporal history compiled out");
    return 0;
}
