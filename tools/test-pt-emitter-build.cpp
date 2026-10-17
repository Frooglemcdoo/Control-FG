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
static UINT bits(float v){UINT u;std::memcpy(&u,&v,4);return u;}
static float value(UINT u){float v;std::memcpy(&v,&u,4);return v;}
int main(){
    ComPtr<ID3D11Device> d;ComPtr<ID3D11DeviceContext> c;D3D_FEATURE_LEVEL level;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,&level,&c)));
    const D3D_SHADER_MACRO macros[]={{"PT_EMITTER_BUILD_TEST","1"},{nullptr,nullptr}};
    ComPtr<ID3DBlob> code,error;
    const auto hr=D3DCompileFromFile(L"src/shaders/pt_emitter_build.hlsl",macros,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
    if(FAILED(hr)){if(error)std::fprintf(stderr,"%s",static_cast<const char*>(error->GetBufferPointer()));return 1;}
    ComPtr<ID3D11ComputeShader> shader;assert(SUCCEEDED(d->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)));c->CSSetShader(shader.Get(),nullptr,0);
    auto buffer=[&](UINT bytes,UINT bind,UINT misc,UINT stride,const void* data){
        D3D11_BUFFER_DESC b{};b.ByteWidth=bytes;b.BindFlags=bind;b.MiscFlags=misc;b.StructureByteStride=stride;
        D3D11_SUBRESOURCE_DATA initial{};initial.pSysMem=data;ComPtr<ID3D11Buffer> result;
        assert(SUCCEEDED(d->CreateBuffer(&b,data?&initial:nullptr,&result)));return result;
    };
    const UINT indexData[]={0,1,2};const float positions[]={0,0,0,2,0,0,0,2,0};
    const UINT attributes[]={0,4095,4095u<<16};std::array<UINT,84> material{};material[12]=bits(2);material[25]=material[26]=material[27]=bits(1);
    auto indices=buffer(sizeof(indexData),D3D11_BIND_SHADER_RESOURCE,D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,0,indexData);
    auto vertex=buffer(sizeof(attributes),D3D11_BIND_SHADER_RESOURCE,D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,0,attributes);
    auto position=buffer(sizeof(positions),D3D11_BIND_SHADER_RESOURCE,D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,0,positions);
    auto materials=buffer(sizeof(material),D3D11_BIND_SHADER_RESOURCE,D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,336,material.data());
    ComPtr<ID3D11ShaderResourceView> views[4];ID3D11Buffer* resources[]={indices.Get(),vertex.Get(),position.Get(),materials.Get()};
    const UINT sizes[]={3,3,9};
    for(UINT i=0;i<4;++i){D3D11_SHADER_RESOURCE_VIEW_DESC s{};
        if(i<3){s.ViewDimension=D3D11_SRV_DIMENSION_BUFFEREX;s.Format=DXGI_FORMAT_R32_TYPELESS;s.BufferEx.NumElements=sizes[i];s.BufferEx.Flags=D3D11_BUFFEREX_SRV_FLAG_RAW;}
        else{s.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;s.Buffer.NumElements=1;}
        assert(SUCCEEDED(d->CreateShaderResourceView(resources[i],&s,&views[i])));auto* ptr=views[i].Get();c->CSSetShaderResources(i<3?i:8,1,&ptr);
    }
    std::array<UINT,40> constants{};constants[1]=4;constants[2]=4;constants[8]=12;constants[9]=12;constants[10]=12;constants[11]=36;
    constants[12]=constants[17]=constants[22]=bits(1);constants[15]=bits(5);constants[24]=1;constants[25]=1;
    constants[28]=constants[33]=constants[38]=bits(1);constants[31]=bits(-5);
    auto cb=buffer(sizeof(constants),D3D11_BIND_CONSTANT_BUFFER,0,0,constants.data());auto* cbp=cb.Get();c->CSSetConstantBuffers(0,1,&cbp);
    constexpr UINT bytes=16+80;
    auto output=buffer(bytes,D3D11_BIND_UNORDERED_ACCESS,D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,0,nullptr);
    D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};ud.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;ud.Format=DXGI_FORMAT_R32_TYPELESS;ud.Buffer.NumElements=bytes/4;ud.Buffer.Flags=D3D11_BUFFER_UAV_FLAG_RAW;
    ComPtr<ID3D11UnorderedAccessView> uav;assert(SUCCEEDED(d->CreateUnorderedAccessView(output.Get(),&ud,&uav)));auto* up=uav.Get();c->CSSetUnorderedAccessViews(4,1,&up,nullptr);
    D3D11_BUFFER_DESC rd{};rd.ByteWidth=bytes;rd.Usage=D3D11_USAGE_STAGING;rd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Buffer> readback;assert(SUCCEEDED(d->CreateBuffer(&rd,nullptr,&readback)));
    auto run=[&](UINT dispatches){
        const UINT zero[4]={};c->ClearUnorderedAccessViewUint(uav.Get(),zero);c->UpdateSubresource(cb.Get(),0,nullptr,constants.data(),0,0);c->UpdateSubresource(materials.Get(),0,nullptr,material.data(),0,0);
        for(UINT i=0;i<dispatches;++i)c->Dispatch(1,1,1);
        c->CopyResource(readback.Get(),output.Get());D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(c->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
        std::array<UINT,24> r{};std::memcpy(r.data(),mapped.pData,bytes);c->Unmap(readback.Get(),0);return r;
    };
    auto r=run(1);assert(r[0]==1&&r[1]==0&&r[2]==0);
    assert(value(r[4])==5&&value(r[8])==7&&value(r[13])==2);assert(value(r[11])==1&&value(r[18])==1);assert(r[19]==0&&r[20]==1);assert(std::isfinite(value(r[21]))&&value(r[21])>0);
    r=run(2);assert(r[0]==2&&r[2]==1); // Overflow invalidates the complete distribution.
    material[12]=0;r=run(1);assert(r[0]==0&&r[1]==0);
    material[12]=bits(2);constants[3]=1;r=run(1);assert(r[0]==0&&r[1]==1);
    constants[3]=0;constants[11]=12;r=run(1);assert(r[0]==0&&r[1]==1);
    constants[11]=36;constants[12]=constants[17]=constants[22]=0;r=run(1);assert(r[0]==0&&r[1]==0);
    puts("PASS: production emitter extraction on WARP; emission filter, transformed geometry, UV/material/alpha ABI, bounded overflow, malformed records and degenerate triangles");
}
