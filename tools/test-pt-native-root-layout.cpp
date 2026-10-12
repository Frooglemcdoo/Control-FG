#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_surface_pipeline.h"
using Microsoft::WRL::ComPtr;
// Fixture transcribed from captured FrameResources_000.cpp, objects 33/4877.
struct Fixture {
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
    explicit Fixture(bool graphics){
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
int main(){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))));
    PTNativeRoot::Registry registry;
    for(bool graphics:{false,true}){
        Fixture f(graphics);const auto kind=graphics?PTNativeRoot::Kind::graphics:PTNativeRoot::Kind::compute;
        assert(PTNativeRoot::Classify(f.desc)==kind);
        for(UINT n=0;n<f.desc.NumParameters;++n){
            auto& p=f.parameters[n];const auto saved=p;
            p.ShaderVisibility=p.ShaderVisibility==D3D12_SHADER_VISIBILITY_ALL?D3D12_SHADER_VISIBILITY_PIXEL:D3D12_SHADER_VISIBILITY_ALL;
            assert(PTNativeRoot::Classify(f.desc)==PTNativeRoot::Kind::unknown);p=saved;
            if(p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_CBV){
                ++p.Descriptor.ShaderRegister;assert(PTNativeRoot::Classify(f.desc)==PTNativeRoot::Kind::unknown);p=saved;
                ++p.Descriptor.RegisterSpace;assert(PTNativeRoot::Classify(f.desc)==PTNativeRoot::Kind::unknown);p=saved;
            }else{
                for(UINT r=0;r<p.DescriptorTable.NumDescriptorRanges;++r){
                    auto& range=const_cast<D3D12_DESCRIPTOR_RANGE&>(p.DescriptorTable.pDescriptorRanges[r]);const auto original=range;
                    ++range.NumDescriptors;assert(PTNativeRoot::Classify(f.desc)==PTNativeRoot::Kind::unknown);range=original;
                    ++range.RegisterSpace;assert(PTNativeRoot::Classify(f.desc)==PTNativeRoot::Kind::unknown);range=original;
                    ++range.BaseShaderRegister;assert(PTNativeRoot::Classify(f.desc)==PTNativeRoot::Kind::unknown);range=original;
                    range.OffsetInDescriptorsFromTableStart=0;assert(PTNativeRoot::Classify(f.desc)==PTNativeRoot::Kind::unknown);range=original;
                }
            }
        }
        ComPtr<ID3DBlob> blob,error;ComPtr<ID3D12RootSignature> root;
        assert(SUCCEEDED(D3D12SerializeRootSignature(&f.desc,D3D_ROOT_SIGNATURE_VERSION_1_0,&blob,&error)));
        assert(SUCCEEDED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root))));
        assert(!registry.Find(root.Get()));
        assert(registry.Observe(blob->GetBufferPointer(),blob->GetBufferSize(),root.Get(),&D3D12CreateVersionedRootSignatureDeserializer));
        auto proof=registry.Find(root.Get());assert(proof&&proof->Type()==kind&&proof->Matches(root.Get()));
        assert(!registry.Observe(blob->GetBufferPointer(),blob->GetBufferSize(),root.Get(),&D3D12CreateVersionedRootSignatureDeserializer));
        assert(!registry.Observe(blob->GetBufferPointer(),4,root.Get(),&D3D12CreateVersionedRootSignatureDeserializer));
        root.Reset();assert(proof->Root()); // Proof owns the exact root identity.
    }
    puts("PASS: exact native graphics/compute root layouts, per-binding mutations, serialized WARP roots and owned identities");
}
