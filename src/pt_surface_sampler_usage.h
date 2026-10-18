#pragma once
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace PTSurfaceComposition {
// A root signature exposes bindings that individual PSOs need not use. Only
// omit global samplers when absent from EVERY active stage, and the pixel-only
// material array when absent from the pixel program (e.g. projected decals). Missing/stripped reflection keeps the old gate.
inline bool RequiresGlobalBinding(const D3D12_SHADER_BYTECODE& shader,bool sampler) {
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
        if(binding.Space==1&&((binding.Type==D3D_SIT_SAMPLER)==sampler))return true;
    }
    return false;
}
inline UINT SurfaceRootRequirements(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pso) {
    UINT required=0x1002; // pixel table and pixel camera CBV
    if(RequiresGlobalBinding(pso.PS,false))required|=1u<<17;
    for(const auto& shader:{pso.VS,pso.PS,pso.HS,pso.DS,pso.GS})
        if(RequiresGlobalBinding(shader,true))required|=1u<<16;
    return required;
}
}
