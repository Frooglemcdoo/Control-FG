#pragma once
#include <d3d12.h>
// Fixture transcribed from captured FrameResources_000.cpp, objects 33/4877.
struct PTNativeRootFixture {
    D3D12_DESCRIPTOR_RANGE ranges[10][3]{};
    D3D12_ROOT_PARAMETER parameters[18]{};
    D3D12_ROOT_SIGNATURE_DESC desc{};
    void Table(UINT slot,UINT storage,D3D12_SHADER_VISIBILITY visibility,D3D12_DESCRIPTOR_RANGE_TYPE type,UINT count,UINT space){
        ranges[storage][0]={type,count,0,space,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
        parameters[slot].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[slot].ShaderVisibility=visibility;
        parameters[slot].DescriptorTable={1,ranges[storage]};
    }
    void Resource(UINT slot,D3D12_SHADER_VISIBILITY visibility,UINT firstCB){
        ranges[slot][0]={D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,firstCB,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
        ranges[slot][1]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
        ranges[slot][2]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
        parameters[slot].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[slot].ShaderVisibility=visibility;
        parameters[slot].DescriptorTable={3,ranges[slot]};
    }
    void Constant(UINT slot,D3D12_SHADER_VISIBILITY visibility,UINT number){
        parameters[slot].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[slot].ShaderVisibility=visibility;
        parameters[slot].Descriptor={number,0};
    }
    explicit PTNativeRootFixture(bool graphics){
        desc.pParameters=parameters;
        if(graphics){
            desc.NumParameters=18;desc.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
            const D3D12_SHADER_VISIBILITY stages[]={D3D12_SHADER_VISIBILITY_VERTEX,D3D12_SHADER_VISIBILITY_PIXEL,D3D12_SHADER_VISIBILITY_HULL,D3D12_SHADER_VISIBILITY_DOMAIN};
            for(UINT n=0;n<4;++n){Resource(n,stages[n],n<2?4:0);Table(4+n,4+n,stages[n],D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0);
                Constant(8+n,D3D12_SHADER_VISIBILITY_VERTEX,n);Constant(12+n,D3D12_SHADER_VISIBILITY_PIXEL,n);}
            Table(16,8,D3D12_SHADER_VISIBILITY_ALL,D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,1);
            Table(17,9,D3D12_SHADER_VISIBILITY_PIXEL,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,244000,1);
        }else{
            desc.NumParameters=8;Resource(0,D3D12_SHADER_VISIBILITY_ALL,4);
            Table(1,1,D3D12_SHADER_VISIBILITY_ALL,D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0);
            for(UINT n=0;n<4;++n)Constant(2+n,D3D12_SHADER_VISIBILITY_ALL,n);
            Table(6,2,D3D12_SHADER_VISIBILITY_ALL,D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,1);
            Table(7,3,D3D12_SHADER_VISIBILITY_ALL,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,244000,1);
        }
    }
};
