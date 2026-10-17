#define NOMINMAX
#include <windows.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <cstdio>
#include <dxgi1_6.h>
#include "pt-glass-native-root-fixture.h"
#include "../src/pt_emitter_runtime.h"
static bool Binding(ID3D12ShaderReflection* r,const char* name,D3D_SHADER_INPUT_TYPE type,UINT slot,D3D_SRV_DIMENSION dimension) {
    D3D12_SHADER_INPUT_BIND_DESC d{};
    return SUCCEEDED(r->GetResourceBindingDescByName(name,&d)) && d.Type==type &&
        d.BindPoint==slot && d.Space==0 && d.BindCount==1 && d.Dimension==dimension;
}
static int Compile(int mode,ID3D12Device* device,ID3D12RootSignature* root) {
    const bool secondary=mode==1,composite=mode==2;
    D3D_SHADER_MACRO macros[]={{"PT_GLASS_SECONDARY_EVALUATION",secondary?"1":"0"},{"PT_GLASS_COMPOSITE",composite?"1":"0"},{nullptr,nullptr}};
    ID3DBlob* code=nullptr;ID3DBlob* errors=nullptr;
    const HRESULT hr=D3DCompileFromFile(L"src/shaders/pt_glass_native.generated.hlsl",macros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",
        D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS|D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES,0,&code,&errors);
    if(errors){fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr)||!code)return 1;
    ID3D12ShaderReflection* r=nullptr;
    if(FAILED(D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D12ShaderReflection),reinterpret_cast<void**>(&r)))){code->Release();return 2;}
    UINT x=0,y=0,z=0;r->GetThreadGroupSize(&x,&y,&z);
    bool good=x==8 && y==16 && z==1 && Binding(r,"g_rwtRadiance",D3D_SIT_UAV_RWTYPED,0,D3D_SRV_DIMENSION_TEXTURE2D);
    if(secondary){
        good=good&&Binding(r,"ptGlassSecondaryHits",D3D_SIT_STRUCTURED,28,D3D_SRV_DIMENSION_BUFFER);
        good=good&&Binding(r,"PTGlassDispatch",D3D_SIT_CBUFFER,6,D3D_SRV_DIMENSION_UNKNOWN);
        D3D12_SHADER_INPUT_BIND_DESC unused{};
        good=good&&FAILED(r->GetResourceBindingDescByName("g_tMaterialId",&unused));
        good=good&&FAILED(r->GetResourceBindingDescByName("g_tNormal_TexcoordX",&unused));
        good=good&&FAILED(r->GetResourceBindingDescByName("g_tPosition_TexcoordY",&unused));
    }else{
        good=good&&Binding(r,"g_tLinearDepth_HitT",D3D_SIT_TEXTURE,24,D3D_SRV_DIMENSION_TEXTURE2D);
        good=good&&Binding(r,"g_tMaterialId",D3D_SIT_TEXTURE,25,D3D_SRV_DIMENSION_TEXTURE2D);
        good=good&&Binding(r,"g_tNormal_TexcoordX",D3D_SIT_TEXTURE,26,D3D_SRV_DIMENSION_TEXTURE2D);
        good=good&&Binding(r,"g_tPosition_TexcoordY",D3D_SIT_TEXTURE,27,D3D_SRV_DIMENSION_TEXTURE2D);
    }
    if(composite)good=good&&Binding(r,"ptGlassRadiance",D3D_SIT_TEXTURE,29,D3D_SRV_DIMENSION_TEXTURE2D)&&Binding(r,"ptGlassSecondaryHits",D3D_SIT_STRUCTURED,28,D3D_SRV_DIMENSION_BUFFER)&&Binding(r,"PTGlassDispatch",D3D_SIT_CBUFFER,6,D3D_SRV_DIMENSION_UNKNOWN);
    D3D12_SHADER_INPUT_BIND_DESC removed{};
    good=good&&FAILED(r->GetResourceBindingDescByName("ptGlassHistory",&removed))&&
        FAILED(r->GetResourceBindingDescByName("ptGlassDiagnostics",&removed));
    r->Release();
    if(!good){code->Release();fputs("FAIL: glass shader binding contract\n",stderr);return 3;}
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root;
    pipeline.CS={code->GetBufferPointer(),code->GetBufferSize()};
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    const HRESULT pipelineHr=device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&pso));
    if(FAILED(pipelineHr)){printf("FAIL: glass native-root PSO creation 0x%08lX\n",static_cast<unsigned long>(pipelineHr));code->Release();return 6;}
    FILE* out=nullptr;
    if(fopen_s(&out,secondary?"build/pt_glass_secondary.dxbc":composite?"build/pt_glass_composite.dxbc":"build/pt_glass_native.dxbc","wb")||!out){code->Release();return 4;}
    good=fwrite(code->GetBufferPointer(),1,code->GetBufferSize(),out)==code->GetBufferSize();
    good=fclose(out)==0&&good;
    printf("PASS: glass %s cs_5_1; 8x16x1 threads, resource contract, and WARP PSO with recovered native root verified; %zu bytes\n",secondary?"secondary":composite?"composite":"native",code->GetBufferSize());
    code->Release();return good?0:5;
}
int main(){
    using Microsoft::WRL::ComPtr;
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;
    ComPtr<ID3DBlob> serializedRoot;
    ComPtr<ID3D12Device> device;ComPtr<ID3D12RootSignature> root;
    if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))||
       FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)))||
       FAILED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))||
       FAILED(CreateGlassNativeRootFixture(device.Get(),&root,&serializedRoot)))return 7;
    PTNativeRoot::Registry roots;
    if(!roots.Observe(serializedRoot->GetBufferPointer(),serializedRoot->GetBufferSize(),root.Get(),&D3D12CreateVersionedRootSignatureDeserializer))return 10;
    auto layout=roots.Find(root.Get());ComPtr<ID3DBlob> emitterCode;
    if(!layout||FAILED(D3DReadFileToBlob(L"build/pt_emitter_build.dxil",&emitterCode)))return 11;
    PTEmitters::Builder emitterBuilder;
    const auto emitterHr=emitterBuilder.Initialize(device.Get(),*layout,emitterCode->GetBufferPointer(),emitterCode->GetBufferSize());
    if(FAILED(emitterHr)){printf("FAIL: emitter builder production root/PSO 0x%08lX\n",static_cast<unsigned long>(emitterHr));return 12;}
    std::shared_ptr<PTEmitters::Storage> storage;
    if(FAILED(PTEmitters::Storage::Acquire(device.Get(),storage))||!storage->Resource()||!storage->Uav().ptr)return 13;
    storage.reset();
    if(FAILED(PTEmitters::Storage::Acquire(device.Get(),storage)))return 14;
    puts("PASS: production emitter builder extended native root, DXIL PSO, clear descriptor and storage reuse on WARP");
    for(int mode=0;mode<3;++mode){int result=Compile(mode,device.Get(),root.Get());if(result)return result;}
    // Validate the actual packaged GI DXIL against the recovered root, including
    // b4/b5, t21..29, sampler s6 space1 and the private u4..u7 bindings.
    ComPtr<ID3DBlob> gi;ComPtr<ID3D12PipelineState> giPipeline;
    if(FAILED(D3DReadFileToBlob(L"build/pt_terminal_gi_compute.dxil",&gi)))return 8;
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();desc.CS={gi->GetBufferPointer(),gi->GetBufferSize()};
    const HRESULT hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&giPipeline));
    if(FAILED(hr)){printf("FAIL: terminal GI production native-root PSO 0x%08lX\n",static_cast<unsigned long>(hr));return 9;}
    puts("PASS: production terminal GI DXIL creates WARP compute PSO with recovered native root");return 0;
}
