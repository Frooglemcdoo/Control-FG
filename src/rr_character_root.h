#pragma once
inline HRESULT RRCharacterSerializeRoot(ID3DBlob** bytes,ID3DBlob** error) noexcept {
    D3D12_DESCRIPTOR_RANGE ranges[]={
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,6,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,6},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,6,0,7},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,2,0,9}};
    D3D12_ROOT_PARAMETER params[3]{};
    params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={4,ranges};
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor.ShaderRegister=1;
    params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={0,0,5};
    D3D12_ROOT_SIGNATURE_DESC desc{3,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    return D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,bytes,error);
}
