#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using Microsoft::WRL::ComPtr;
static UINT bits(float value){UINT result;std::memcpy(&result,&value,4);return result;}
int main(){
    ComPtr<ID3D11Device> d;ComPtr<ID3D11DeviceContext> c;D3D_FEATURE_LEVEL level;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,&level,&c)));
    ComPtr<ID3DBlob> code,error;
    const D3D_SHADER_MACRO macros[]={{"PT_GI_TEST","1"},{nullptr,nullptr}};
    auto hr=D3DCompileFromFile(L"src/shaders/pt_terminal_gi_compute.hlsl",macros,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
    if(FAILED(hr)){if(error)fprintf(stderr,"%s\n",static_cast<const char*>(error->GetBufferPointer()));else fprintf(stderr,"Shader compile HRESULT=%08lx\n",static_cast<unsigned long>(hr));return 1;}
    ComPtr<ID3D11ComputeShader> shader;assert(SUCCEEDED(d->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)));
    std::array<ComPtr<ID3D11ShaderResourceView>,9> views;
    std::array<ComPtr<ID3D11Buffer>,3> buffers;
    auto structured=[&](UINT slot,UINT bytes,UINT stride,const void* data){
        D3D11_BUFFER_DESC b{};b.ByteWidth=bytes;b.BindFlags=D3D11_BIND_SHADER_RESOURCE;b.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;b.StructureByteStride=stride;
        D3D11_SUBRESOURCE_DATA initial{};initial.pSysMem=data;
        assert(SUCCEEDED(d->CreateBuffer(&b,&initial,&buffers[slot])));assert(SUCCEEDED(d->CreateShaderResourceView(buffers[slot].Get(),nullptr,&views[slot])));
    };
    std::array<UINT,40> volumes{};
    for(UINT v=0;v<2;++v){auto* p=volumes.data()+v*20;p[0]=p[4]=p[8]=bits(1);p[9]=p[10]=p[11]=bits(1);p[15]=bits(1);p[16]=bits(1);}
    std::array<UINT,8> top{};top.fill(0x80000000u);std::array<UINT,4> node{};
    structured(0,160,80,volumes.data());structured(1,32,4,top.data());structured(2,16,16,node.data());
    for(UINT i=0;i<6;++i){
        std::array<float,2048> pixels{};for(UINT j=0;j<512;++j){pixels[j*4]=float(i+1);pixels[j*4+1]=float(i+1)*2;pixels[j*4+2]=float(i+1)*3;}
        D3D11_TEXTURE3D_DESC td{};td.Width=td.Height=td.Depth=8;td.MipLevels=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=pixels.data();data.SysMemPitch=128;data.SysMemSlicePitch=1024;
        ComPtr<ID3D11Texture3D> texture;assert(SUCCEEDED(d->CreateTexture3D(&td,&data,&texture)));assert(SUCCEEDED(d->CreateShaderResourceView(texture.Get(),nullptr,&views[3+i])));
    }
    std::array<UINT,6144> transforms{};
    for(UINT i=0;i<2;++i){auto* t=transforms.data()+i*16;t[0]=t[5]=t[10]=bits(1);t[12]=t[13]=t[14]=2;}
    std::array<UINT,16> params{};params[0]=params[1]=params[2]=bits(.125f);params[3]=1;params[12]=bits(3.14159265358979323846f);params[13]=1;
    ComPtr<ID3D11Buffer> cb4,cb5;
    auto constant=[&](UINT bytes,ID3D11Buffer** buffer){D3D11_BUFFER_DESC b{};b.ByteWidth=bytes;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;assert(SUCCEEDED(d->CreateBuffer(&b,nullptr,buffer)));};
    constant(24576,&cb4);constant(64,&cb5);
    std::array<ComPtr<ID3D11Buffer>,4> work;
    std::array<ComPtr<ID3D11UnorderedAccessView>,4> outputs;
    const UINT byteSizes[]={256,256,256,512};
    for(UINT i=0;i<4;++i){
        D3D11_BUFFER_DESC b{};b.ByteWidth=byteSizes[i];b.BindFlags=D3D11_BIND_UNORDERED_ACCESS;b.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        assert(SUCCEEDED(d->CreateBuffer(&b,nullptr,&work[i])));
        D3D11_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R32_TYPELESS;u.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;u.Buffer.NumElements=byteSizes[i]/4;u.Buffer.Flags=D3D11_BUFFER_UAV_FLAG_RAW;
        assert(SUCCEEDED(d->CreateUnorderedAccessView(work[i].Get(),&u,&outputs[i])));
    }
    ComPtr<ID3D11Buffer> readback;
    D3D11_BUFFER_DESC rb{};rb.ByteWidth=512;rb.Usage=D3D11_USAGE_STAGING;rb.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(d->CreateBuffer(&rb,nullptr,&readback)));
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    ComPtr<ID3D11SamplerState> sampler;assert(SUCCEEDED(d->CreateSamplerState(&sd,&sampler)));
    ID3D11ShaderResourceView* srvs[9];for(UINT i=0;i<9;++i)srvs[i]=views[i].Get();
    c->CSSetShader(shader.Get(),nullptr,0);c->CSSetShaderResources(21,9,srvs);c->CSSetSamplers(6,1,sampler.GetAddressOf());c->CSSetConstantBuffers(4,1,cb4.GetAddressOf());c->CSSetConstantBuffers(5,1,cb5.GetAddressOf());
    for(UINT test=0;test<8;++test){
        std::array<UINT,64> geometry{},weights{},metrics{};std::array<UINT,128> raw{};
        auto p=params;p[14]=8;p[15]=1;metrics[8]=1;
        if(test==1)metrics[8]=0;
        if(test==2)metrics[8]=2;
        if(test==3)p[13]=0;
        if(test==4)p[14]=9; // Mismatched buffer dimensions cannot touch outputs.
        for(UINT pixel=0;pixel<8;++pixel){
            auto* g=geometry.data()+pixel*8;g[0]=g[1]=g[2]=bits(.5f);g[3]=bits(1);g[4]=bits(1);
            auto* w=weights.data()+pixel*8;w[0]=bits(.5f);w[1]=bits(2);w[2]=bits(3);w[4]=bits(.25f);w[5]=bits(.5f);w[6]=bits(1);
            auto* o=raw.data()+pixel*16;o[0]=bits(10);o[1]=bits(20);o[2]=bits(30);o[3]=bits(1);
            o[4]=bits(40);o[5]=bits(50);o[6]=bits(60);o[7]=bits(7);
            for(UINT j=8;j<16;++j)o[j]=bits(float(j));
            if(pixel==1)g[3]=0; // Invalid terminal request, stale payload underneath.
            if(pixel==2)o[3]=0; // Native-owned pixel.
            if(pixel==3)g[0]=bits(3); // Outside volume.
            if(pixel==4)w[0]=0x7fc00000u; // Invalid weight.
            if(pixel==5)w[4]=bits(2); // Invalid lobe fraction.
            if(pixel==6)g[4]=bits(-1); // Opposite directional brick.
            if(test==5)g[3]=0;
            if(test==6)w[0]=w[1]=w[2]=0;
            if(test==7)w[4]=w[5]=w[6]=0; // All tail belongs to specular first lobe.
        }
        c->UpdateSubresource(cb4.Get(),0,nullptr,transforms.data(),0,0);c->UpdateSubresource(cb5.Get(),0,nullptr,p.data(),0,0);
        c->UpdateSubresource(work[0].Get(),0,nullptr,geometry.data(),0,0);c->UpdateSubresource(work[1].Get(),0,nullptr,weights.data(),0,0);
        c->UpdateSubresource(work[2].Get(),0,nullptr,metrics.data(),0,0);c->UpdateSubresource(work[3].Get(),0,nullptr,raw.data(),0,0);
        ID3D11UnorderedAccessView* uavs[4];for(UINT i=0;i<4;++i)uavs[i]=outputs[i].Get();
        c->CSSetUnorderedAccessViews(4,4,uavs,nullptr);c->Dispatch(2,1,1);
        ID3D11UnorderedAccessView* nulls[4]{};c->CSSetUnorderedAccessViews(4,4,nulls,nullptr);
        c->CopyResource(readback.Get(),work[3].Get());D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(c->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
        const auto* got=static_cast<const float*>(mapped.pData);
        for(UINT pixel=0;pixel<8;++pixel)for(UINT word=0;word<16;++word){
            float expected;std::memcpy(&expected,&raw[pixel*16+word],4);
            bool valid=(test==0||test==7)&&(pixel==0||pixel==6||pixel==7||(test==7&&pixel==5));
            if(valid&&(word<3||(word>=4&&word<7))){
                UINT channel=word%4;float weight[]={.5f,2,3},fraction[]={.25f,.5f,1};
                float f=test==7?0:fraction[channel];if(word>=4)f=1-f;
                expected+=float(channel+1)*(pixel==6?2.f:1.f)*weight[channel]*f;
            }
            if(!std::isfinite(got[pixel*16+word])||std::abs(got[pixel*16+word]-expected)>1e-4f){printf("FAIL compute test=%u pixel=%u word=%u got=%g expected=%g\n",test,pixel,word,got[pixel*16+word],expected);return 1;}
        }
        c->Unmap(readback.Get(),0);
    }
    assert(SUCCEEDED(d->GetDeviceRemovedReason()));puts("PASS: production terminal GI compute handoff, exact lobe weights, admission/replay/probe guards, stale/invalid requests, bounds and guide preservation");
}
