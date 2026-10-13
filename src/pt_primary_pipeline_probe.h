#pragma once
#include "pt_glass_capture_policy.h"
// Bounded CPU-only evidence. Candidate names select bytes to COLLECT, never a
// pipeline to execute. The reviewed primary hash gate remains unchanged.
namespace PTPrimaryPipelineProbe {
inline bool Candidate(const unsigned char* bytes,std::size_t size){
    using namespace PTGlassCapture;
    if(!bytes||size<32||size>kMaximumBytes||std::memcmp(bytes,"DXBC",4)||Word(bytes+24)!=size)return false;
    const auto count=Word(bytes+28);if(!count||count>(size-32)/4)return false;
    bool compute=false,lighting=false;
    for(unsigned i=0;i<count;++i){
        const auto offset=Word(bytes+32+i*4);if(offset<32+count*4||offset>size-8)return false;
        const auto length=Word(bytes+offset+4);if(length>size-offset-8)return false;
        const auto* data=bytes+offset+8;
        if((!std::memcmp(bytes+offset,"SHEX",4)||!std::memcmp(bytes+offset,"SHDR",4))&&length>=4)compute=(Word(data)>>16)==5;
        if(!std::memcmp(bytes+offset,"RDEF",4))lighting=Contains(data,length,"g_tGBuffer1")&&Contains(data,length,"g_tGBuffer2")&&Contains(data,length,"deferredlight_constants");
    }
    return compute&&lighting;
}
#ifdef PT_PRIMARY_PROBE_HOST
struct Entry {
    Microsoft::WRL::ComPtr<IUnknown> identity;
    char sha[65]{};UINT index=0;std::size_t bytes=0;
    bool candidate=false,used=false;std::vector<unsigned char> code;
};
static std::mutex mutex;
static std::map<IUnknown*,std::shared_ptr<Entry>> entries;
static std::size_t held=0;
static unsigned unknown=0,limited=0;
static void Created(const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* object){
    if(!desc||!object||!desc->CS.pShaderBytecode||!desc->CS.BytecodeLength||desc->CS.BytecodeLength>PTGlassCapture::kMaximumBytes)return;
    auto e=std::make_shared<Entry>();e->bytes=desc->CS.BytecodeLength;
    if(FAILED(object->QueryInterface(IID_PPV_ARGS(&e->identity))))return;
    const auto* code=static_cast<const unsigned char*>(desc->CS.pShaderBytecode);
    if(!PTReflectionSha256(code,e->bytes,e->sha))return;
    e->candidate=Candidate(code,e->bytes);
    std::lock_guard<std::mutex> guard(mutex);if(entries.count(e->identity.Get()))return;
    if(entries.size()>=1024){if(++limited==1)Log("PT_PRIMARY_PSO_PROBE_LIMIT kind=entries");return;}
    e->index=static_cast<UINT>(entries.size());
    if(e->candidate&&e->bytes<=12u*1024u*1024u-held){e->code.assign(code,code+e->bytes);held+=e->bytes;}
    Log("PT_PRIMARY_PSO_CREATED index=%u identity=%p sha256=%s bytes=%llu candidate=%u retained=%u",e->index,e->identity.Get(),e->sha,static_cast<unsigned long long>(e->bytes),unsigned(e->candidate),unsigned(!e->code.empty()));
    entries.emplace(e->identity.Get(),e);
}
static void Used(ID3D12PipelineState* pipeline,UINT x,UINT y,UINT z){
    if(!pipeline)return;Microsoft::WRL::ComPtr<IUnknown> identity;
    if(FAILED(pipeline->QueryInterface(IID_PPV_ARGS(&identity))))return;
    std::shared_ptr<Entry> e;std::vector<unsigned char> code;
    {std::lock_guard<std::mutex> guard(mutex);const auto found=entries.find(identity.Get());
        if(found==entries.end()){if(++unknown<=8)Log("PT_PRIMARY_PSO_UNTRACKED identity=%p groups=%ux%ux%u",identity.Get(),x,y,z);return;}
        e=found->second;if(e->used)return;e->used=true;code.swap(e->code);held-=code.size();}
    Log("PT_PRIMARY_PSO_USED index=%u identity=%p sha256=%s bytes=%llu candidate=%u groups=%ux%ux%u",e->index,identity.Get(),e->sha,static_cast<unsigned long long>(e->bytes),unsigned(e->candidate),x,y,z);
    if(code.empty())return;
    Log("PT_PRIMARY_SHADER_BEGIN index=%u bytes=%llu sha256=%s",e->index,static_cast<unsigned long long>(code.size()),e->sha);
    constexpr std::size_t chunk=384;
    for(std::size_t offset=0;offset<code.size();offset+=chunk){
        const auto count=(std::min)(chunk,code.size()-offset);char hex[chunk*2+1]{};
        for(std::size_t n=0;n<count;++n)sprintf_s(hex+n*2,3,"%02X",code[offset+n]);
        Log("PT_PRIMARY_SHADER_CHUNK index=%u offset=%llu bytes=%llu hex=%s",e->index,static_cast<unsigned long long>(offset),static_cast<unsigned long long>(count),hex);
    }
    Log("PT_PRIMARY_SHADER_END index=%u bytes=%llu sha256=%s",e->index,static_cast<unsigned long long>(code.size()),e->sha);
}
#endif
}
