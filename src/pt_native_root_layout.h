#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <mutex>
#include <map>
#include <vector>
// Exact layouts reconstructed from the reviewed native root signatures (PIX
// objects 33 and 4877). A shader hash alone does not prove root parameter order.
namespace PTNativeRoot {
enum class Kind {unknown,graphics,compute};
inline bool Table(const D3D12_ROOT_PARAMETER& p,D3D12_SHADER_VISIBILITY visibility,
                  D3D12_DESCRIPTOR_RANGE_TYPE type,UINT count,UINT base,UINT space){
    if(p.ParameterType!=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE||p.ShaderVisibility!=visibility||
       p.DescriptorTable.NumDescriptorRanges!=1||!p.DescriptorTable.pDescriptorRanges)return false;
    const auto& r=p.DescriptorTable.pDescriptorRanges[0];
    return r.RangeType==type&&r.NumDescriptors==count&&r.BaseShaderRegister==base&&r.RegisterSpace==space&&
           r.OffsetInDescriptorsFromTableStart==D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}
inline bool Resources(const D3D12_ROOT_PARAMETER& p,D3D12_SHADER_VISIBILITY visibility,UINT firstCB){
    if(p.ParameterType!=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE||p.ShaderVisibility!=visibility||
       p.DescriptorTable.NumDescriptorRanges!=3||!p.DescriptorTable.pDescriptorRanges)return false;
    const D3D12_DESCRIPTOR_RANGE_TYPE types[]={D3D12_DESCRIPTOR_RANGE_TYPE_CBV,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_DESCRIPTOR_RANGE_TYPE_UAV};
    const UINT counts[]={5,36,8},bases[]={firstCB,0,0};
    for(UINT n=0;n<3;++n){const auto& r=p.DescriptorTable.pDescriptorRanges[n];
        if(r.RangeType!=types[n]||r.NumDescriptors!=counts[n]||r.BaseShaderRegister!=bases[n]||r.RegisterSpace||
           r.OffsetInDescriptorsFromTableStart!=D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)return false;
    }
    return true;
}
inline bool Constant(const D3D12_ROOT_PARAMETER& p,D3D12_SHADER_VISIBILITY visibility,UINT number){
    return p.ParameterType==D3D12_ROOT_PARAMETER_TYPE_CBV&&p.ShaderVisibility==visibility&&
           p.Descriptor.ShaderRegister==number&&p.Descriptor.RegisterSpace==0;
}
inline Kind Classify(const D3D12_ROOT_SIGNATURE_DESC& d){
    if(!d.pParameters||d.NumStaticSamplers)return Kind::unknown;
    const auto* p=d.pParameters;
    if(d.NumParameters==8&&d.Flags==D3D12_ROOT_SIGNATURE_FLAG_NONE){
        const auto all=D3D12_SHADER_VISIBILITY_ALL;
        if(!Resources(p[0],all,4)||!Table(p[1],all,D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0)||
           !Table(p[6],all,D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,0,1)||
           !Table(p[7],all,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,244000,0,1))return Kind::unknown;
        for(UINT n=0;n<4;++n)if(!Constant(p[2+n],all,n))return Kind::unknown;
        return Kind::compute;
    }
    const auto graphicsFlags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    if(d.NumParameters!=18||d.Flags!=graphicsFlags)return Kind::unknown;
    const D3D12_SHADER_VISIBILITY stages[]={D3D12_SHADER_VISIBILITY_VERTEX,D3D12_SHADER_VISIBILITY_PIXEL,D3D12_SHADER_VISIBILITY_HULL,D3D12_SHADER_VISIBILITY_DOMAIN};
    for(UINT n=0;n<4;++n){
        if(!Resources(p[n],stages[n],n<2?4:0)||!Table(p[4+n],stages[n],D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0)||
           !Constant(p[8+n],D3D12_SHADER_VISIBILITY_VERTEX,n)||!Constant(p[12+n],D3D12_SHADER_VISIBILITY_PIXEL,n))return Kind::unknown;
    }
    return Table(p[16],D3D12_SHADER_VISIBILITY_ALL,D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,0,1)&&
           Table(p[17],D3D12_SHADER_VISIBILITY_PIXEL,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,244000,0,1)?Kind::graphics:Kind::unknown;
}
class Layout {
    friend class Registry;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<IUnknown> identity;
    Kind kind=Kind::unknown;
    std::vector<unsigned char> serialized;
public:
    const std::vector<unsigned char>& Serialized()const noexcept{return serialized;}
    Kind Type()const noexcept{return kind;}
    ID3D12RootSignature* Root()const noexcept{return root.Get();}
    bool Matches(ID3D12RootSignature* candidate)const {
        if(!candidate)return false;Microsoft::WRL::ComPtr<IUnknown> key;
        return SUCCEEDED(candidate->QueryInterface(IID_PPV_ARGS(&key)))&&key.Get()==identity.Get();
    }
};
class Registry {
    std::mutex mutex;
    std::map<IUnknown*,std::shared_ptr<const Layout>> entries;
public:
    using Deserialize=HRESULT(WINAPI*)(LPCVOID,SIZE_T,REFIID,void**);
    bool Observe(const void* blob,SIZE_T bytes,IUnknown* created,Deserialize deserialize){
        if(!blob||!bytes||bytes>1024*1024||!created||!deserialize)return false;
        Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> decoder;
        if(FAILED(deserialize(blob,bytes,IID_PPV_ARGS(&decoder))))return false;
        const auto* d=decoder->GetUnconvertedRootSignatureDesc();
        // The reviewed game uses version 1.0. Do not discard version 1.1's data
        // volatility promises by converting an unreviewed signature to 1.0.
        if(!d||d->Version!=D3D_ROOT_SIGNATURE_VERSION_1_0)return false;
        const auto kind=Classify(d->Desc_1_0);if(kind==Kind::unknown)return false;
        auto layout=std::make_shared<Layout>();layout->kind=kind;
        const auto* raw=static_cast<const unsigned char*>(blob);layout->serialized.assign(raw,raw+bytes);
        if(FAILED(created->QueryInterface(IID_PPV_ARGS(&layout->root)))||FAILED(created->QueryInterface(IID_PPV_ARGS(&layout->identity))))return false;
        std::lock_guard<std::mutex> guard(mutex);if(entries.size()>=64)return false;
        auto* key=layout->identity.Get();return entries.emplace(key,std::move(layout)).second;
    }
    std::shared_ptr<const Layout> Find(ID3D12RootSignature* root){
        if(!root)return {};Microsoft::WRL::ComPtr<IUnknown> identity;
        if(FAILED(root->QueryInterface(IID_PPV_ARGS(&identity))))return {};
        std::lock_guard<std::mutex> guard(mutex);const auto found=entries.find(identity.Get());
        return found==entries.end()?nullptr:found->second;
    }
};
}
