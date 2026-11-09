#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include <cstdio>
#include "pt-glass-native-root-fixture.h"
using Microsoft::WRL::ComPtr;
int main(){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;
    ComPtr<ID3D12Device> device;ComPtr<ID3D12RootSignature> root;
    if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))||FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)))||
       FAILED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))||FAILED(CreateGlassNativeRootFixture(device.Get(),&root)))return 1;
    ComPtr<ID3DBlob> code,errors;
    const HRESULT hr=D3DCompileFromFile(L"src/shaders/opaque_native.generated.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS|D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES,0,&code,&errors);
    if(errors)fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);
    if(FAILED(hr))return 2;
    ComPtr<ID3D12ShaderReflection> reflect;
    if(FAILED(D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),IID_PPV_ARGS(&reflect))))return 3;
    struct Expected {const char* name;D3D_SHADER_INPUT_TYPE type;UINT slot;D3D_SRV_DIMENSION dimension;};
    const Expected bindings[]={
        {"opaqueHits",D3D_SIT_STRUCTURED,29,D3D_SRV_DIMENSION_BUFFER},
        {"OpaqueDispatch",D3D_SIT_CBUFFER,7,D3D_SRV_DIMENSION_UNKNOWN},
        {"rtreflection",D3D_SIT_CBUFFER,6,D3D_SRV_DIMENSION_UNKNOWN},
        {"g_tMaterialId",D3D_SIT_TEXTURE,26,D3D_SRV_DIMENSION_TEXTURE2DARRAY},
        {"g_tNormal_TexcoordX",D3D_SIT_TEXTURE,27,D3D_SRV_DIMENSION_TEXTURE2DARRAY},
        {"g_tPosition_TexcoordY",D3D_SIT_TEXTURE,28,D3D_SRV_DIMENSION_TEXTURE2DARRAY},
        {"g_rwtReflectionTarget",D3D_SIT_UAV_RWTYPED,0,D3D_SRV_DIMENSION_TEXTURE2D}};
    for(const auto& e:bindings){D3D12_SHADER_INPUT_BIND_DESC d{};
        if(FAILED(reflect->GetResourceBindingDescByName(e.name,&d))||d.Type!=e.type||d.BindPoint!=e.slot||d.Space!=0||d.Dimension!=e.dimension){printf("FAIL binding %s\n",e.name);return 4;}}
    UINT x=0,y=0,z=0;reflect->GetThreadGroupSize(&x,&y,&z);if(x!=8||y!=16||z!=1)return 5;
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code->GetBufferPointer(),code->GetBufferSize()};
    ComPtr<ID3D12PipelineState> pso;const HRESULT ph=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso));
    if(FAILED(ph)){printf("FAIL native-root opaque PSO 0x%08lX\n",static_cast<unsigned long>(ph));return 6;}
    FILE* out=nullptr;if(fopen_s(&out,"build/opaque_reflection_composite.dxbc","wb")||!out)return 7;
    const bool written=fwrite(code->GetBufferPointer(),1,code->GetBufferSize(),out)==code->GetBufferSize();fclose(out);
    if(!written)return 8;
    printf("PASS: opaque composite cs_5_1, native array/sample bindings, private t29/b7, 8x16 threads and recovered-root WARP PSO; %zu bytes\n",code->GetBufferSize());return 0;
}
