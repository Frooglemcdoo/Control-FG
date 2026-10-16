#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <array>
#include <cstring>
#include "../src/pt_guide_admission.h"
using Microsoft::WRL::ComPtr;
int main(){
    using PTGuideAdmission::Reject;
    static_assert(Reject(true,false,false,false,false,true,true)==1,"recorded is not submitted");
    static_assert(Reject(true,true,false,false,false,true,true)==0,"earlier armed producer");
    static_assert(Reject(true,false,true,false,false,true,true)==0,"COPY-gated producer");
    static_assert(Reject(false,true,false,false,false,true,true)==1,"missing producer");
    static_assert(Reject(true,true,false,true,false,true,true)==1,"cancelled producer");
    static_assert(Reject(true,true,false,false,true,true,true)==2,"replay");
    static_assert(Reject(true,true,false,false,false,false,true)==4,"wrong queue");
    static_assert(Reject(true,true,false,false,false,true,false)==8,"scene rejected");
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context)));
    ComPtr<ID3DBlob> code,error;
    HRESULT hr=D3DCompileFromFile(L"src/shaders/rr_live_guides.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"GenerateLiveGuides","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
    if(FAILED(hr)&&error)puts(static_cast<const char*>(error->GetBufferPointer()));assert(SUCCEEDED(hr));
    ComPtr<ID3D11ComputeShader> shader;assert(SUCCEEDED(device->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)));
    std::array<ComPtr<ID3D11ShaderResourceView>,7> views;
    std::array<float,32> g1{},g2{},env{};
    for(UINT i=0;i<8;++i){g1[i*4]=g1[i*4+1]=g1[i*4+2]=.5f;g2[i*4+1]=.04f;env[i*4]=.5f;env[i*4+1]=.125f;}
    D3D11_TEXTURE2D_DESC td{};td.Width=8;td.Height=1;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    for(UINT i=0;i<3;++i){
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=i==0?g1.data():(i==1?g2.data():env.data());data.SysMemPitch=128;
        ComPtr<ID3D11Texture2D> texture;assert(SUCCEEDED(device->CreateTexture2D(&td,&data,&texture)));
        assert(SUCCEEDED(device->CreateShaderResourceView(texture.Get(),nullptr,&views[i==2?3:i])));
    }
    const UINT packed[2]={0x0a0a0a00,0};D3D11_BUFFER_DESC bd{};bd.ByteWidth=8;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=8;
    D3D11_SUBRESOURCE_DATA initial{};initial.pSysMem=packed;ComPtr<ID3D11Buffer> material;assert(SUCCEEDED(device->CreateBuffer(&bd,&initial,&material)));assert(SUCCEEDED(device->CreateShaderResourceView(material.Get(),nullptr,&views[2])));
    std::array<float,128> raw{};std::array<float,64> surface{};
    for(UINT i=0;i<8;++i){raw[i*16+3]=surface[i*8+3]=1;raw[i*16+8]=.2f;raw[i*16+9]=.4f;raw[i*16+10]=.6f;}
    raw[16+8]=raw[16+9]=raw[16+10]=0;surface[2*8+3]=0;raw[3*16+3]=0;
    const UINT wrong=1;std::memcpy(&surface[4*8+5],&wrong,4);
    raw[5*16+8]=std::numeric_limits<float>::quiet_NaN();raw[6*16+8]=1.5f;raw[7*16+8]=raw[7*16+9]=raw[7*16+10]=1;
    for(UINT i=0;i<2;++i){
        bd={};bd.ByteWidth=i?256:512;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        initial.pSysMem=i?surface.data():raw.data();ComPtr<ID3D11Buffer> buffer;assert(SUCCEEDED(device->CreateBuffer(&bd,&initial,&buffer)));
        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=DXGI_FORMAT_R32_TYPELESS;sd.ViewDimension=D3D11_SRV_DIMENSION_BUFFEREX;sd.BufferEx.NumElements=bd.ByteWidth/4;sd.BufferEx.Flags=D3D11_BUFFEREX_SRV_FLAG_RAW;
        assert(SUCCEEDED(device->CreateShaderResourceView(buffer.Get(),&sd,&views[4+i])));
    }
    bd={};bd.ByteWidth=16;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    ComPtr<ID3D11Buffer> guard;assert(SUCCEEDED(device->CreateBuffer(&bd,nullptr,&guard)));
    D3D11_SHADER_RESOURCE_VIEW_DESC gd{};gd.Format=DXGI_FORMAT_R32_TYPELESS;gd.ViewDimension=D3D11_SRV_DIMENSION_BUFFEREX;gd.BufferEx.NumElements=4;gd.BufferEx.Flags=D3D11_BUFFEREX_SRV_FLAG_RAW;
    assert(SUCCEEDED(device->CreateShaderResourceView(guard.Get(),&gd,&views[6])));
    std::array<ComPtr<ID3D11Texture2D>,3> outputs;std::array<ComPtr<ID3D11UnorderedAccessView>,3> uavs;
    td.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
    for(UINT i=0;i<3;++i){assert(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&outputs[i])));assert(SUCCEEDED(device->CreateUnorderedAccessView(outputs[i].Get(),nullptr,&uavs[i])));}
    td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> readback;assert(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&readback)));
    bd={};bd.ByteWidth=16;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;ComPtr<ID3D11Buffer> constants;assert(SUCCEEDED(device->CreateBuffer(&bd,nullptr,&constants)));
    D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampler.MaxLOD=D3D11_FLOAT32_MAX;ComPtr<ID3D11SamplerState> state;assert(SUCCEEDED(device->CreateSamplerState(&sampler,&state)));
    ID3D11ShaderResourceView* srvs[7];ID3D11UnorderedAccessView* targets[3];for(UINT i=0;i<7;++i)srvs[i]=views[i].Get();for(UINT i=0;i<3;++i)targets[i]=uavs[i].Get();
    context->CSSetShader(shader.Get(),nullptr,0);context->CSSetShaderResources(0,7,srvs);context->CSSetSamplers(0,1,state.GetAddressOf());context->CSSetConstantBuffers(0,1,constants.GetAddressOf());
    std::array<std::array<float,32>,3> baseline{};
    // Off -> PT guides -> off: fallback pixels and normal/specular guides stay exact.
    for(UINT mode=0;mode<5;++mode){
        const UINT admission[4]={(mode==1||mode==3)?1u:0u,0,0,0};
        context->UpdateSubresource(guard.Get(),0,nullptr,admission,0,0);
        const UINT data[]={1,(mode>=1&&mode<=3)?7u:3u,8,1};context->UpdateSubresource(constants.Get(),0,nullptr,data,0,0);
        context->CSSetUnorderedAccessViews(0,3,targets,nullptr);context->Dispatch(1,1,1);
        ID3D11UnorderedAccessView* nulls[3]{};context->CSSetUnorderedAccessViews(0,3,nulls,nullptr);
        for(UINT output=0;output<3;++output){
            context->CopyResource(readback.Get(),outputs[output].Get());D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
            const auto* values=static_cast<const float*>(mapped.pData);
            if(mode==0)std::memcpy(baseline[output].data(),values,128);
            for(UINT pixel=0;pixel<8;++pixel)for(UINT channel=0;channel<4;++channel){
                float expected=baseline[output][pixel*4+channel];
                if((mode==1||mode==3)&&output==1&&channel<3&&(pixel==0||pixel==1||pixel==7))expected=raw[pixel*16+8+channel];
                assert(std::isfinite(values[pixel*4+channel])&&std::abs(values[pixel*4+channel]-expected)<1e-6f);
            }
            context->Unmap(readback.Get(),0);
        }
    }
    assert(SUCCEEDED(device->GetDeviceRemovedReason()));puts("PASS: submission-order policy and GPU admission rejection/re-enable; actual RR guide shader uses PT diffuse albedo including black; uncovered, mismatched, nonfinite and invalid pixels preserve native guides; normal/specular and PT-off unchanged");
}
