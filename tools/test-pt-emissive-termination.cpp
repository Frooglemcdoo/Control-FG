#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cassert>
#include <cmath>
#include <cstdio>
using Microsoft::WRL::ComPtr;
int main(){
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context)));
    ComPtr<ID3DBlob> code,error;
    assert(SetCurrentDirectoryW(L"src/shaders"));
    const auto hr=D3DCompileFromFile(L"../../tools/test-pt-emissive-termination.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_SKIP_OPTIMIZATION|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error);
    if(FAILED(hr)){if(error)fprintf(stderr,"%s\n",static_cast<const char*>(error->GetBufferPointer()));return 1;}
    ComPtr<ID3D11ComputeShader> shader;assert(SUCCEEDED(device->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)));
    D3D11_BUFFER_DESC desc{};desc.ByteWidth=128;desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;desc.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;desc.StructureByteStride=16;
    ComPtr<ID3D11Buffer> output,readback;assert(SUCCEEDED(device->CreateBuffer(&desc,nullptr,&output)));
    ComPtr<ID3D11UnorderedAccessView> uav;assert(SUCCEEDED(device->CreateUnorderedAccessView(output.Get(),nullptr,&uav)));
    desc.BindFlags=0;desc.MiscFlags=0;desc.StructureByteStride=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(device->CreateBuffer(&desc,nullptr,&readback)));
    context->CSSetShader(shader.Get(),nullptr,0);context->CSSetUnorderedAccessViews(0,1,uav.GetAddressOf(),nullptr);context->Dispatch(1,1,1);
    ID3D11UnorderedAccessView* none=nullptr;context->CSSetUnorderedAccessViews(0,1,&none,nullptr);context->CopyResource(readback.Get(),output.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
    const auto* values=static_cast<const float*>(mapped.pData);
    for(unsigned n=0;n<8;++n)for(unsigned c=0;c<4;++c){
        float expected=c==3?(n==5?0.f:1.f):((n==0||n==6||n==7)?float(c+2)*(n==7?.5f:1.f):0.f);
        if(!std::isfinite(values[n*4+c])||std::abs(values[n*4+c]-expected)>1e-4f){printf("FAIL case=%u channel=%u actual=%g expected=%g\n",n,c,values[n*4+c],expected);return 1;}
    }
    context->Unmap(readback.Get(),0);assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    puts("PASS: production path integrator retains secondary emission once, weights throughput, rejects invalid/backface/unsupported primary");
}
