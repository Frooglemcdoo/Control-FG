#pragma once
#include <d3d12.h>
#include <wrl/client.h>
// Test fixture reconstructed from Steam d3d_rmdwin10_f.dll (SHA cceb99cb...),
// factory RVA 0x18c10, installed at renderer-device +0x70 by RVA 0x2b049.
// The compiler's COMPUTE_ROOT_SIGNATURE string omits runtime root parameter 7.
// Production integration must borrow the captured root, not instantiate this.
inline HRESULT CreateGlassNativeRootFixture(ID3D12Device* device,ID3D12RootSignature** result){
    D3D12_DESCRIPTOR_RANGE ranges[6]{};
    ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ranges[2]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ranges[3]={D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ranges[4]={D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,0,1,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ranges[5]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,0x3b920,0,1,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    D3D12_ROOT_PARAMETER p[8]{};
    p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[0].DescriptorTable={3,ranges};
    p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={1,&ranges[3]};
    for(UINT i=2;i<6;++i){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i].Descriptor={i-2,0};}
    p[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[6].DescriptorTable={1,&ranges[4]};
    p[7].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[7].DescriptorTable={1,&ranges[5]};
    D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=8;desc.pParameters=p;
    Microsoft::WRL::ComPtr<ID3DBlob> bytes,errors;
    HRESULT hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&errors);
    if(SUCCEEDED(hr))hr=device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(result));
    return hr;
}
