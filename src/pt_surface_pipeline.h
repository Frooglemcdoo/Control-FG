#pragma once
#include "pt_surface_composition.h"
#include "pt_native_root_layout.h"
#include "pt_surface_sampler_usage.h"
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
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root,composedRoot;
    std::shared_ptr<const PTNativeRoot::Layout> layout;
    std::uint64_t required=0;
    UINT rootRequirements=0x31002;bool vertexCamera=false;
public:
    static HRESULT Create(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* description,
                          IUnknown* original,const char* verifiedSha,const std::shared_ptr<const PTNativeRoot::Layout>& rootLayout,
                          CreateGraphics create,std::shared_ptr<Pipeline>& output,const char* vertexSha=nullptr){
        output.reset();
        if(!device||!description||!original||!create||!description->pRootSignature)return E_INVALIDARG;
        if(description->SampleDesc.Count!=1)return S_FALSE;
        if(!rootLayout||rootLayout->Type()!=PTNativeRoot::Kind::graphics||!rootLayout->Matches(description->pRootSignature))return E_INVALIDARG;
        std::vector<unsigned char> program;
        if(!Patch(static_cast<const unsigned char*>(description->PS.pShaderBytecode),description->PS.BytecodeLength,verifiedSha,program))return S_FALSE;
        auto pending=std::make_shared<Pipeline>();
        pending->rootRequirements=SurfaceRootRequirements(*description);
        // These two reviewed foliage PS variants do not read ViewToWorld.
        // Their matching reviewed VS does, in vertex b0/root8. Never use
        // unpopulated pixel CBV lanes as a camera identity.
        if(!std::strcmp(verifiedSha,"3755ae5f574c639ee79de158a309414dec754dcc4964ccfa477322f7ef1f658a")||
           !std::strcmp(verifiedSha,"68b095104f50891b21ebd4ff28c3ec038f1d6062c84f4f4568929a5037bc16a7")){
            if(!vertexSha||std::strcmp(vertexSha,"a954d6e56122c5c0008775c37371d1bf240c6349c89c3397c455df655cdc1ea1"))return S_FALSE;
            pending->vertexCamera=true;pending->rootRequirements|=1u<<8;
        }
        for(const auto& profile:Profiles)if(!std::strcmp(profile.sha,verifiedSha))pending->required=profile.required;
        HRESULT hr=original->QueryInterface(IID_PPV_ARGS(&pending->native));if(FAILED(hr))return hr;
        Microsoft::WRL::ComPtr<ID3D12Device> owner;
        Microsoft::WRL::ComPtr<IUnknown> expected,observed;
        if(FAILED(device->QueryInterface(IID_PPV_ARGS(&expected)))||FAILED(pending->native->GetDevice(IID_PPV_ARGS(&owner)))||
           FAILED(owner.As(&observed))||expected.Get()!=observed.Get())return E_INVALIDARG;
        // Append a root SRV and width constant: no native descriptor allocation,
        // heap switch, or per-draw table copying is necessary for PT radiance.
        Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> decoder;
        const auto& bytes=rootLayout->Serialized();
        hr=D3D12CreateVersionedRootSignatureDeserializer(bytes.data(),bytes.size(),IID_PPV_ARGS(&decoder));if(FAILED(hr))return hr;
        const auto* decoded=decoder->GetUnconvertedRootSignatureDesc();
        if(!decoded||decoded->Version!=D3D_ROOT_SIGNATURE_VERSION_1_0||
           PTNativeRoot::Classify(decoded->Desc_1_0)!=PTNativeRoot::Kind::graphics)return E_INVALIDARG;
        auto desc=decoded->Desc_1_0;
        std::vector<D3D12_ROOT_PARAMETER> parameters(desc.pParameters,desc.pParameters+desc.NumParameters);
        D3D12_ROOT_PARAMETER radiance{};radiance.ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;
        radiance.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;radiance.Descriptor={0,2};parameters.push_back(radiance);
        D3D12_ROOT_PARAMETER extent{};extent.ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        extent.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;extent.Constants={0,2,2};parameters.push_back(extent);
        D3D12_ROOT_PARAMETER consumer{};consumer.ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;
        consumer.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;consumer.Descriptor={1,2};parameters.push_back(consumer);
        desc.NumParameters=static_cast<UINT>(parameters.size());desc.pParameters=parameters.data();
        Microsoft::WRL::ComPtr<ID3DBlob> serialized,error;
        hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1_0,&serialized,&error);if(FAILED(hr))return hr;
        hr=device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&pending->composedRoot));if(FAILED(hr))return hr;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC adapted=*description;
        adapted.pRootSignature=pending->composedRoot.Get();
        adapted.PS={program.data(),program.size()};adapted.CachedPSO={};
        hr=create(device,&adapted,IID_PPV_ARGS(&pending->composed));if(FAILED(hr))return hr;
        pending->root=description->pRootSignature;pending->layout=rootLayout;output=std::move(pending);return S_OK;
    }
    ID3D12PipelineState* Native()const noexcept{return native.Get();}
    ID3D12PipelineState* Composed()const noexcept{return composed.Get();}
    ID3D12RootSignature* Root()const noexcept{return root.Get();}
    ID3D12RootSignature* ComposedRoot()const noexcept{return composedRoot.Get();}
    std::uint64_t Required()const noexcept{return required;}
    bool VertexCamera()const noexcept{return vertexCamera;}
    UINT RootRequirements()const noexcept{return rootRequirements;}
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
