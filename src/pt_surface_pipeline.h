#pragma once
#include "pt_surface_composition.h"
#include "pt_native_root_layout.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <mutex>
#include <map>

namespace PTSurfaceComposition {
using CreateGraphics=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_GRAPHICS_PIPELINE_STATE_DESC*,REFIID,void**);
// The native PSO is always created and returned unchanged. This companion owns
// a separate PSO with the same root, shaders, fixed-function state and formats;
// only the reviewed pixel program and its stale cached-PSO blob differ.
class Pipeline {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> native,composed;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    std::shared_ptr<const PTNativeRoot::Layout> layout;
public:
    static HRESULT Create(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* description,
                          IUnknown* original,const char* verifiedSha,const std::shared_ptr<const PTNativeRoot::Layout>& rootLayout,
                          CreateGraphics create,std::shared_ptr<Pipeline>& output){
        output.reset();
        if(!device||!description||!original||!create||!description->pRootSignature)return E_INVALIDARG;
        if(!rootLayout||rootLayout->Type()!=PTNativeRoot::Kind::graphics||!rootLayout->Matches(description->pRootSignature))return E_INVALIDARG;
        std::vector<unsigned char> program;
        if(!Patch(static_cast<const unsigned char*>(description->PS.pShaderBytecode),description->PS.BytecodeLength,verifiedSha,program))return S_FALSE;
        auto pending=std::make_shared<Pipeline>();
        HRESULT hr=original->QueryInterface(IID_PPV_ARGS(&pending->native));if(FAILED(hr))return hr;
        Microsoft::WRL::ComPtr<ID3D12Device> owner;
        Microsoft::WRL::ComPtr<IUnknown> expected,observed;
        if(FAILED(device->QueryInterface(IID_PPV_ARGS(&expected)))||FAILED(pending->native->GetDevice(IID_PPV_ARGS(&owner)))||
           FAILED(owner.As(&observed))||expected.Get()!=observed.Get())return E_INVALIDARG;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC adapted=*description;
        adapted.PS={program.data(),program.size()};adapted.CachedPSO={};
        hr=create(device,&adapted,IID_PPV_ARGS(&pending->composed));if(FAILED(hr))return hr;
        pending->root=description->pRootSignature;pending->layout=rootLayout;output=std::move(pending);return S_OK;
    }
    ID3D12PipelineState* Native()const noexcept{return native.Get();}
    ID3D12PipelineState* Composed()const noexcept{return composed.Get();}
    ID3D12RootSignature* Root()const noexcept{return root.Get();}
};

// Native pipeline identities remain owned, so a destroyed/reused raw address
// cannot resolve to an unrelated companion. Session-size cap bounds retention.
class Pipelines {
    std::mutex mutex;
    std::map<IUnknown*,std::pair<Microsoft::WRL::ComPtr<IUnknown>,std::shared_ptr<Pipeline>>> entries;
public:
    bool Insert(const std::shared_ptr<Pipeline>& pipeline){
        if(!pipeline)return false;
        Microsoft::WRL::ComPtr<IUnknown> identity;
        if(FAILED(pipeline->Native()->QueryInterface(IID_PPV_ARGS(&identity))))return false;
        std::lock_guard<std::mutex> guard(mutex);
        if(entries.size()>=512||entries.count(identity.Get()))return false;
        auto* key=identity.Get();entries.emplace(key,std::make_pair(std::move(identity),pipeline));return true;
    }
    std::shared_ptr<Pipeline> Find(ID3D12PipelineState* native){
        if(!native)return {};
        Microsoft::WRL::ComPtr<IUnknown> identity;
        if(FAILED(native->QueryInterface(IID_PPV_ARGS(&identity))))return {};
        std::lock_guard<std::mutex> guard(mutex);const auto found=entries.find(identity.Get());
        return found==entries.end()?nullptr:found->second.second;
    }
};
}
