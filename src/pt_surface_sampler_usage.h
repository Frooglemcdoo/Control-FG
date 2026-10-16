#pragma once
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace PTSurfaceComposition {
// A root signature exposes bindings that individual PSOs need not use. Only
// omit the global sampler requirement when reflection proves it absent from
// EVERY active graphics stage. Missing/stripped reflection keeps the old gate.
inline bool RequiresGlobalSamplers(const D3D12_SHADER_BYTECODE& shader) {
    if(!shader.BytecodeLength)return false;
    if(!shader.pShaderBytecode)return true;
    using Reflect=HRESULT(WINAPI*)(LPCVOID,SIZE_T,REFIID,void**);
    static const auto reflect=[](){
        const auto module=LoadLibraryW(L"d3dcompiler_47.dll");
        return module?reinterpret_cast<Reflect>(GetProcAddress(module,"D3DReflect")):nullptr;
    }();
    if(!reflect)return true;
    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
    if(FAILED(reflect(shader.pShaderBytecode,shader.BytecodeLength,IID_PPV_ARGS(&reflection))))return true;
    D3D12_SHADER_DESC description{};
    if(FAILED(reflection->GetDesc(&description)))return true;
    for(UINT n=0;n<description.BoundResources;++n){
        D3D12_SHADER_INPUT_BIND_DESC binding{};
        if(FAILED(reflection->GetResourceBindingDesc(n,&binding)))return true;
        if(binding.Type==D3D_SIT_SAMPLER&&binding.Space==1)return true;
    }
    return false;
}
inline UINT SurfaceRootRequirements(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pso) {
    UINT required=0x21002; // pixel table, pixel camera CBV, material texture table
    for(const auto& shader:{pso.VS,pso.PS,pso.HS,pso.DS,pso.GS})
        if(RequiresGlobalSamplers(shader))required|=1u<<16;
    return required;
}
}
