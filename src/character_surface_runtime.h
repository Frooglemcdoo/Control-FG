#pragma once
#include <vector>
#include <algorithm>
namespace character_surface {
#include "../build/character_surface.generated.h"
inline constexpr unsigned kind=0x73460000;
// 0=native,1=R351,2=R353 HQ,3=HQ with near history. Capture phase overrides never mutate this choice.
inline std::atomic<unsigned> quality{3};
inline int SelectedVariant(unsigned phase,unsigned quality) noexcept {
    if(phase==0)return quality==1?1:(quality==2 || quality==3)?0:-1;
    return (phase==2 || phase==4)?0:phase==3?1:phase==5?2:phase==6?3:phase==7?4:phase==9?5:-1;
}
inline std::atomic<uint64_t> filteredBinds{0};
inline std::atomic<unsigned> phase{0},matched{0},ready{0};
inline std::atomic<uint64_t> binds[12]{},applied[12]{};
inline std::atomic<uint64_t> groupBinds[3][12]{},groupApplied[3][12]{};
inline GUID VariantGuid(unsigned index) noexcept {
    return GUID{0xf2713460+index,0x71b4,0x4aea,{0xa7,0x31,0xe6,0x31,0x44,0xc2,0x12,0x54}};
}
// Reflection/debug chunks and container checksum are not shader identity.
// Executable declarations/instructions and both linkage signatures must match.
inline D3D12_SHADER_BYTECODE Chunk(const D3D12_SHADER_BYTECODE& b,const char* name) noexcept {
    if(!b.pShaderBytecode || b.BytecodeLength<32)return {};
    const auto* p=static_cast<const unsigned char*>(b.pShaderBytecode);
    auto read=[&](size_t o){unsigned v=0;std::memcpy(&v,p+o,4);return v;};
    if(std::memcmp(p,"DXBC",4) || read(24)!=b.BytecodeLength)return {};
    const unsigned count=read(28);
    if(count>(b.BytecodeLength-32)/4)return {};
    D3D12_SHADER_BYTECODE found{};
    for(unsigned i=0;i<count;++i){
        const size_t o=read(32+i*4);
        if(o<32+size_t(count)*4 || o>b.BytecodeLength-8)return {};
        const size_t n=read(o+4);if(n>b.BytecodeLength-o-8)return {};
        if(!std::memcmp(p+o,name,4)){if(found.pShaderBytecode)return {};found={p+o+8,n};}
    }
    return found;
}
inline bool Equivalent(const D3D12_SHADER_BYTECODE& b,const D3D12_SHADER_BYTECODE& known) noexcept {
    for(const char* name:{"SHEX","ISGN","OSGN"}){
        const auto a=Chunk(b,name),k=Chunk(known,name);
        if(!a.pShaderBytecode || !k.pShaderBytecode || a.BytecodeLength!=k.BytecodeLength ||
           std::memcmp(a.pShaderBytecode,k.pShaderBytecode,a.BytecodeLength))return false;
    }
    return true;
}
inline int Family(const D3D12_SHADER_BYTECODE& b) noexcept {
    for(unsigned i=0;i<kCharacterFamilyCount;++i)if(Equivalent(b,kCharacterOriginals[i]))return int(i);
    return -1;
}
// Bounded startup-only CPU evidence; no capture readbacks or per-frame copies.
inline SRWLOCK candidateLock=SRWLOCK_INIT;
inline std::vector<std::vector<unsigned char>> candidates;
inline size_t candidateBytes=0;
inline std::atomic<unsigned> observed{0},candidateDropped{0};
inline void Remember(const D3D12_SHADER_BYTECODE& b) noexcept {
    ++observed;
    if(!b.pShaderBytecode || b.BytecodeLength<12000 || b.BytecodeLength>60000)return;
    const auto code=Chunk(b,"SHEX"),known=Chunk({kCharacterSurface0,sizeof(kCharacterSurface0)},"SHEX");
    const char marker[]="g_bCharacterLightRigs";
    const auto* begin=static_cast<const unsigned char*>(b.pShaderBytecode);
    const bool named=std::search(begin,begin+b.BytecodeLength,marker,marker+sizeof(marker)-1)!=begin+b.BytecodeLength;
    const bool declarations=code.BytecodeLength>=264 && known.BytecodeLength>=264 &&
        !std::memcmp(static_cast<const unsigned char*>(code.pShaderBytecode)+8,static_cast<const unsigned char*>(known.pShaderBytecode)+8,256);
    if(!named && !declarations)return;
    if(!TryAcquireSRWLockExclusive(&candidateLock)){++candidateDropped;return;}
    const auto* p=static_cast<const unsigned char*>(b.pShaderBytecode);
    bool duplicate=false;for(const auto& c:candidates)if(c.size()==b.BytecodeLength && !std::memcmp(c.data(),p,c.size())){duplicate=true;break;}
    if(!duplicate){
        if(candidates.size()<32 && candidateBytes+b.BytecodeLength<=1024*1024){
            try{candidates.emplace_back(p,p+b.BytecodeLength);candidateBytes+=b.BytecodeLength;}catch(...){++candidateDropped;}
        }else ++candidateDropped;
    }
    ReleaseSRWLockExclusive(&candidateLock);
}
inline unsigned ExportCandidates(const wchar_t* folder) noexcept {
    AcquireSRWLockShared(&candidateLock);unsigned saved=0;
    for(unsigned i=0;i<candidates.size();++i){
        wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\pixel-candidate-%02u.dxbc",folder,i);
        HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,0,nullptr);
        if(f!=INVALID_HANDLE_VALUE){DWORD n=0;if(WriteFile(f,candidates[i].data(),DWORD(candidates[i].size()),&n,nullptr)&&n==candidates[i].size())++saved;CloseHandle(f);}
    }
    ReleaseSRWLockShared(&candidateLock);return saved;
}
template<class Create>
inline void Observe(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc,
                    IUnknown* native,Create create) noexcept {
    if(!create || !native || !desc)return;
    Remember(desc->PS);
    const int family=Family(desc->PS);if(family<0)return;
    ID3D12PipelineState* owner=nullptr;
    if(FAILED(native->QueryInterface(IID_PPV_ARGS(&owner))))return;
    unsigned tag=0;UINT size=sizeof(tag);
    if(SUCCEEDED(owner->GetPrivateData(kKindGuid,&size,&tag)) && tag==kind){owner->Release();return;}
    ++matched;
    ID3D12PipelineState* variants[kCharacterVariantCount]{};
    HRESULT hr=S_OK;
    for(unsigned i=0;i<kCharacterVariantCount && SUCCEEDED(hr);++i){
        auto copy=*desc;copy.PS=kCharacterVariants[family][i];copy.CachedPSO={nullptr,0};
        hr=create(device,&copy,IID_PPV_ARGS(&variants[i]));
    }
    for(unsigned i=0;i<kCharacterVariantCount && SUCCEEDED(hr);++i)hr=owner->SetPrivateDataInterface(VariantGuid(i),variants[i]);
    const unsigned group=kCharacterGroups[family];
    if(SUCCEEDED(hr))hr=owner->SetPrivateData(VariantGuid(16),sizeof(group),&group);
    if(SUCCEEDED(hr))hr=owner->SetPrivateData(kKindGuid,sizeof(kind),&kind);
    if(SUCCEEDED(hr))++ready;
    else for(unsigned i=0;i<kCharacterVariantCount;++i)owner->SetPrivateDataInterface(VariantGuid(i),nullptr);
    for(auto* p:variants)if(p)p->Release();
    Trace("CHARACTER_SURFACE_R356 matched=%u ready=%u hr=0x%08lX",matched.load(),ready.load(),static_cast<unsigned long>(hr));
    owner->Release();
}
}
