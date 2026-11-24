#pragma once
#include "../build/gi_character_material.h"
namespace RRCharacterMaterial {
using Microsoft::WRL::ComPtr;
static ComPtr<ID3D12RootSignature> root;
static ComPtr<ID3D12PipelineState> pipeline;
static bool Prepare(ID3D12Device* device){
    if(pipeline)return true;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT support{DXGI_FORMAT_R16G16B16A16_FLOAT};
    constexpr UINT required=D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD|D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
    if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support)))||
       (support.Support2&required)!=required)return false;
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0},
                                     {D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,3}};
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable={2,ranges};
    D3D12_ROOT_SIGNATURE_DESC desc{1,&parameter,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;ComPtr<ID3D12RootSignature> sig;
    if(FAILED(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error))||
       FAILED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&sig))))return false;
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=sig.Get();pd.CS={kGICharacterMaterial,sizeof(kGICharacterMaterial)};
    if(FAILED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline))))return false;
    root=std::move(sig);return true;
}
static void Dispatch(ID3D12GraphicsCommandList* list,D3D12_GPU_DESCRIPTOR_HANDLE table,
                     ID3D12Resource* destination,UINT w,UINT h){
    D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=destination;
    list->ResourceBarrier(1,&order);
    list->SetComputeRootSignature(root.Get());list->SetPipelineState(pipeline.Get());
    list->SetComputeRootDescriptorTable(0,table);list->Dispatch((w+15)/16,(h+15)/16,1);
    list->ResourceBarrier(1,&order);
}
}
