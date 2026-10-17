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
    auto hr=D3DCompileFromFile(L"tools/test-pt-terminal-gi.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
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
    D3D11_BUFFER_DESC b{};b.ByteWidth=128;b.BindFlags=D3D11_BIND_UNORDERED_ACCESS;b.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;b.StructureByteStride=16;
    ComPtr<ID3D11Buffer> output,readback;assert(SUCCEEDED(d->CreateBuffer(&b,nullptr,&output)));
    ComPtr<ID3D11UnorderedAccessView> uav;assert(SUCCEEDED(d->CreateUnorderedAccessView(output.Get(),nullptr,&uav)));
    b.BindFlags=0;b.MiscFlags=0;b.StructureByteStride=0;b.Usage=D3D11_USAGE_STAGING;b.CPUAccessFlags=D3D11_CPU_ACCESS_READ;assert(SUCCEEDED(d->CreateBuffer(&b,nullptr,&readback)));
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    ComPtr<ID3D11SamplerState> sampler;assert(SUCCEEDED(d->CreateSamplerState(&sd,&sampler)));
    ID3D11ShaderResourceView* srvs[9];for(UINT i=0;i<9;++i)srvs[i]=views[i].Get();
    c->CSSetShader(shader.Get(),nullptr,0);c->CSSetShaderResources(21,9,srvs);c->CSSetSamplers(6,1,sampler.GetAddressOf());c->CSSetConstantBuffers(4,1,cb4.GetAddressOf());c->CSSetConstantBuffers(5,1,cb5.GetAddressOf());
    // Valid leaf, disabled, invalid count/tree/index/brick, overlapping volumes,
    // internal-child and parent-brick paths, rotated basis and scale/bias.
    for(UINT test=0;test<14;++test){
        auto p=params;auto v=volumes;auto t=top;auto n=node;auto tr=transforms;
        if(test==1)p[13]=0;
        if(test==2)p[3]=385;
        if(test==3)t[0]=100;
        if(test==4)v[12]=8;
        if(test==5)v[14]=0xffffffffu;
        if(test==6)p[3]=2;
        if(test==7){t[0]=0;n[0]=n[1]=1u<<13;}
        if(test==8)t[0]=0;
        if(test==9){v[0]=0;v[1]=bits(1);v[3]=bits(1);v[4]=0;}
        if(test==10){p[12]=bits(1.57079632679489661923f);p[4]=p[5]=p[6]=bits(2);}
        if(test==11)v[9]=v[10]=v[11]=0;
        if(test==12)tr[0]=0x7fc00000u;
        if(test==13)p[3]=0;
        c->UpdateSubresource(cb4.Get(),0,nullptr,tr.data(),0,0);c->UpdateSubresource(cb5.Get(),0,nullptr,p.data(),0,0);
        c->UpdateSubresource(buffers[0].Get(),0,nullptr,v.data(),0,0);c->UpdateSubresource(buffers[1].Get(),0,nullptr,t.data(),0,0);c->UpdateSubresource(buffers[2].Get(),0,nullptr,n.data(),0,0);
        c->CSSetUnorderedAccessViews(0,1,uav.GetAddressOf(),nullptr);c->Dispatch(1,1,1);ID3D11UnorderedAccessView* nullView=nullptr;c->CSSetUnorderedAccessViews(0,1,&nullView,nullptr);
        c->CopyResource(readback.Get(),output.Get());D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(c->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
        auto* values=static_cast<const float*>(mapped.pData);
        for(UINT pixel=0;pixel<8;++pixel)for(UINT channel=0;channel<4;++channel){
            float expected=pixel<6?float(pixel+1):2.f;
            if(test==9&&pixel<4)expected=float((pixel^2u)+1);
            expected*=float(channel+1);if(test==10)expected=(expected+2)*.5f;
            if(pixel==7||(test>=1&&test<=5)||test>=11)expected=0;
            if(channel==3)expected=pixel==7||test==2||test==5||test>=11?0.f:1.f;
            if(!std::isfinite(values[pixel*4+channel])||std::abs(values[pixel*4+channel]-expected)>1e-4f){printf("FAIL test=%u pixel=%u channel=%u actual=%g expected=%g\n",test,pixel,channel,values[pixel*4+channel],expected);return 1;}
        }
        c->Unmap(readback.Get(),0);
    }
    assert(SUCCEEDED(d->GetDeviceRemovedReason()));puts("PASS: terminal GI GPU directional lobes, overlap normalization, native scale/bias, basis rotation, leaf/internal/parent paths, disabled/outside/malformed inputs");
}
