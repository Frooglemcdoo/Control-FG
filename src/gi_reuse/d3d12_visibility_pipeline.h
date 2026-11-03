#pragma once
// Prepare an augmented state object without changing native hit groups or SBTs.
// Caller supplies the hash-admitted native contact-shadow state descriptor.
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <array>
#include <cstring>
#include <limits>
namespace gi_reuse {
struct VisibilityIdentifierPair {
    std::array<unsigned char,32> native{}, replacement{};
};
class VisibilityPipeline {
    Microsoft::WRL::ComPtr<ID3D12StateObject> state_;
    Microsoft::WRL::ComPtr<ID3D12Resource> records_;
    std::vector<VisibilityIdentifierPair> identifiers_;
public:
    using Create = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC*,REFIID,void**);
    ID3D12StateObject* State() const noexcept { return state_.Get(); }
    ID3D12Resource* Records() const noexcept { return records_.Get(); }
    const std::vector<VisibilityIdentifierPair>& Identifiers() const noexcept { return identifiers_; }
    // Returns a completely prepared object or leaves this object unchanged.
    HRESULT Initialize(ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC& native,
                       D3D12_SHADER_BYTECODE library,Create create) noexcept {
        if(!device||!create||!library.pShaderBytecode||!library.BytecodeLength||
           native.Type!=D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE||
           !native.pSubobjects||!native.NumSubobjects||native.NumSubobjects>65536) return E_INVALIDARG;
        try {
            std::vector<UINT> groupIndices;
            for(UINT i=0;i<native.NumSubobjects;++i) {
                const auto& sub=native.pSubobjects[i];
                if(!sub.pDesc) return E_INVALIDARG;
                // Collections/configured pipelines require a separate reflection route.
                if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) return E_NOTIMPL;
                if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP) {
                    const auto& group=*static_cast<const D3D12_HIT_GROUP_DESC*>(sub.pDesc);
                    if(group.Type!=D3D12_HIT_GROUP_TYPE_TRIANGLES||group.IntersectionShaderImport||!group.HitGroupExport) return E_NOTIMPL;
                    groupIndices.push_back(i);
                }
            }
            if(groupIndices.empty()||groupIndices.size()>4096) return E_INVALIDARG;
            const UINT count=static_cast<UINT>(groupIndices.size());
            std::vector<std::wstring> groupNames(count),hitNames(count);
            std::vector<D3D12_HIT_GROUP_DESC> groups(count);
            std::vector<D3D12_EXPORT_DESC> exports(count+2);
            exports[0]={L"controlGIConnectionRaygen",L"giConnectionVisibility",D3D12_EXPORT_FLAG_NONE};
            exports[1]={L"controlGIConnectionMiss",L"giConnectionMiss",D3D12_EXPORT_FLAG_NONE};
            for(UINT i=0;i<count;++i) {
                groupNames[i]=L"controlGIConnectionGroup"+std::to_wstring(i);
                hitNames[i]=L"controlGIConnectionHit"+std::to_wstring(i);
                const auto& old=*static_cast<const D3D12_HIT_GROUP_DESC*>(native.pSubobjects[groupIndices[i]].pDesc);
                groups[i]={groupNames[i].c_str(),D3D12_HIT_GROUP_TYPE_TRIANGLES,
                    old.AnyHitShaderImport,hitNames[i].c_str(),nullptr};
                exports[i+2]={hitNames[i].c_str(),L"giConnectionBlocked",D3D12_EXPORT_FLAG_NONE};
            }
            std::vector<D3D12_STATE_SUBOBJECT> objects(native.pSubobjects,native.pSubobjects+native.NumSubobjects);
            objects.reserve(native.NumSubobjects+count+1);
            std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(native.NumSubobjects);
            std::vector<std::vector<LPCWSTR>> names(native.NumSubobjects);
            // Preserve each native export association; duplicate group associations
            // for the new hit groups so their native any-hit resource ABI is retained.
            for(UINT i=0;i<native.NumSubobjects;++i) {
                if(objects[i].Type!=D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) continue;
                auto& association=associations[i];
                association=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[i].pDesc);
                if(association.NumExports&&!association.pExports) return E_INVALIDARG;
                UINT target=native.NumSubobjects;
                for(UINT j=0;j<native.NumSubobjects;++j) if(association.pSubobjectToAssociate==native.pSubobjects+j) target=j;
                if(target==native.NumSubobjects) return E_INVALIDARG;
                if(association.NumExports) names[i].assign(association.pExports,association.pExports+association.NumExports);
                for(UINT j=0;j<association.NumExports;++j) {
                    if(!association.pExports[j]) return E_INVALIDARG;
                    for(UINT k=0;k<count;++k) {
                        const auto& old=*static_cast<const D3D12_HIT_GROUP_DESC*>(native.pSubobjects[groupIndices[k]].pDesc);
                        if(!wcscmp(association.pExports[j],old.HitGroupExport)) names[i].push_back(groupNames[k].c_str());
                        if(old.ClosestHitShaderImport&&!wcscmp(association.pExports[j],old.ClosestHitShaderImport)) names[i].push_back(hitNames[k].c_str());
                    }
                    // Native spot raygen root/config covers the new raygen/miss.
                    if(!wcscmp(association.pExports[j],L"spotRayGeneration")) {
                        const auto type=objects[target].Type;
                        if(type==D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE) return E_NOTIMPL;
                        if(type==D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE||type==D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG) {
                            names[i].push_back(exports[0].Name);names[i].push_back(exports[1].Name);
                        }
                    }
                }
                association.NumExports=static_cast<UINT>(names[i].size());association.pExports=names[i].data();
                association.pSubobjectToAssociate=objects.data()+target;objects[i].pDesc=&association;
            }
            D3D12_DXIL_LIBRARY_DESC added{library,static_cast<UINT>(exports.size()),exports.data()};
            objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&added});
            for(auto& group:groups) objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&group});
            auto desc=native;desc.NumSubobjects=static_cast<UINT>(objects.size());desc.pSubobjects=objects.data();
            Microsoft::WRL::ComPtr<ID3D12StateObject> state;
            HRESULT hr=create(device,&desc,IID_PPV_ARGS(&state));if(FAILED(hr)) return hr;
            if(!state) return E_UNEXPECTED;
            Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
            hr=state.As(&properties);if(FAILED(hr)) return hr;
            std::vector<VisibilityIdentifierPair> identifiers(count);
            for(UINT i=0;i<count;++i) {
                const auto& old=*static_cast<const D3D12_HIT_GROUP_DESC*>(native.pSubobjects[groupIndices[i]].pDesc);
                const void* from=properties->GetShaderIdentifier(old.HitGroupExport);
                const void* to=properties->GetShaderIdentifier(groupNames[i].c_str());
                if(!from||!to) return E_UNEXPECTED;
                std::memcpy(identifiers[i].native.data(),from,32);std::memcpy(identifiers[i].replacement.data(),to,32);
            }
            std::array<unsigned char,128> bytes{};
            for(UINT i=0;i<2;++i) {
                const void* id=properties->GetShaderIdentifier(exports[i].Name);if(!id) return E_UNEXPECTED;
                std::memcpy(bytes.data()+i*64,id,32);
            }
            D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
            D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=128;buffer.Height=1;
            buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Microsoft::WRL::ComPtr<ID3D12Resource> records;
            hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&records));
            if(FAILED(hr)) return hr;
            void* mapped=nullptr;D3D12_RANGE empty{};hr=records->Map(0,&empty,&mapped);if(FAILED(hr)) return hr;
            std::memcpy(mapped,bytes.data(),bytes.size());records->Unmap(0,nullptr);
            state_=std::move(state);records_=std::move(records);identifiers_=std::move(identifiers);return S_OK;
        } catch(...) { return E_OUTOFMEMORY; }
    }
};
}
