#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_surface_pipeline.h"
using Microsoft::WRL::ComPtr;
#include "pt-native-root-fixture.h"
int main(){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))));
    PTNativeRoot::Registry registry;
    for(bool graphics:{false,true}){
        PTNativeRootFixture f(graphics);const auto kind=graphics?PTNativeRoot::Kind::graphics:PTNativeRoot::Kind::compute;
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
