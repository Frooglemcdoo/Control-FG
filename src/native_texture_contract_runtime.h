#pragma once
#include "native_texture_layout.h"
#include <wrl/client.h>
namespace native_texture_contract {
using U64=std::uint64_t;
inline U64 Field(const void* p,unsigned offset){return native_loader_stages::Field(p,offset);}
inline unsigned Dword(const void* p,unsigned offset){unsigned value=0;memcpy(&value,static_cast<const unsigned char*>(p)+offset,4);return value;}
struct Snapshot {
    U64 owner=0,texture=0,container=0,resource=0,id=0,offset=0,bytes=0;
    unsigned level=0,state=~0u,heap=~0u,format=0,nativeFlags=0,mipState=~0u;
    bool layoutValid=false;native_texture_layout::Layout layout{};
};
inline thread_local Snapshot* pending=nullptr;
struct Entry {Snapshot data{};bool occupied=false,read=false,upload=false;};
struct Publication {U64 texture=0,id=0,owner=0;unsigned level=0;bool occupied=false;};
inline Entry entries[256]{};inline Publication publications[256]{};
inline std::atomic_flag guard=ATOMIC_FLAG_INIT;
inline std::atomic<U64> requests{0},sampleAttempts{0},captured{0},constructed{0},reads{0},uploads{0},queued{0},dropped{0},invalid{0};
inline unsigned PubIndex(U64 texture,unsigned level){return static_cast<unsigned>(((texture>>4)^(U64(level)*0x9e3779b9u))&255);}
struct Unlock {~Unlock(){guard.clear(std::memory_order_release);}};
inline bool installed=false;
using Request=std::int64_t(*)(void*,const char*,void*,U64,U64,char*,int,int,bool,void*);
using Upload=void(*)(void*,unsigned,void*);
using Publish=void(*)(void*,void*);
inline Request request=nullptr;inline Upload upload=nullptr;inline Publish publish=nullptr;
inline RRIndirectCallPatch requestPatch;
inline NativeCpuCallPatch callPatches[3]{};
inline void Inspect(void* texture,unsigned level,Snapshot& out){
    if(!texture||level>=Dword(texture,0xf0)||level>=32){++invalid;return;}
    auto* levels=reinterpret_cast<unsigned char*>(Field(texture,0xe8));if(!levels){++invalid;return;}
    auto* entry=levels+U64(level)*0x28;out.mipState=Dword(entry,0x20);
    auto* container=reinterpret_cast<void*>(Field(entry,0x10));if(!container){++invalid;return;}
    out.container=reinterpret_cast<U64>(container);
    auto* native=reinterpret_cast<void*>(Field(container,8));if(!native){++invalid;return;}
    auto* resource=reinterpret_cast<ID3D12Resource*>(Field(native,0x88));if(!resource){++invalid;return;}
    out.resource=reinterpret_cast<U64>(resource);out.nativeFlags=Dword(native,0x38);
    auto* state=reinterpret_cast<void*>(Field(native,0x50));out.state=Dword(state?state:static_cast<unsigned char*>(native)+0x40,0x20);
    D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
    if(SUCCEEDED(resource->GetHeapProperties(&heap,&flags)))out.heap=static_cast<unsigned>(heap.Type);
    const auto desc=resource->GetDesc();out.format=static_cast<unsigned>(desc.Format);
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    if(SUCCEEDED(resource->GetDevice(IID_PPV_ARGS(&device))))out.layoutValid=native_texture_layout::Build(device.Get(),desc,out.layout);
}
// Injection point also lets the ABI fixture use copied synthetic metadata without
// pretending a byte array is a live D3D12 COM resource.
using Inspector=void(*)(void*,unsigned,Snapshot&);inline Inspector inspector=&Inspect;
inline std::int64_t RequestHook(void* owner,const char* name,void* listener,U64 offset,U64 bytes,char* buffer,int thread,int priority,bool flag,void* user){
    const DWORD incoming=GetLastError();const U64 ordinal=++requests;
    Snapshot snapshot{};bool sample=false;
    if(ordinal<=8||ordinal%64==0)sample=sampleAttempts.fetch_add(1)<96;
    if(sample){
        ++captured;snapshot.owner=reinterpret_cast<U64>(owner);snapshot.texture=reinterpret_cast<U64>(listener);
        snapshot.offset=offset;snapshot.bytes=bytes;snapshot.level=static_cast<unsigned>(reinterpret_cast<U64>(user));
        inspector(listener,snapshot.level,snapshot);
    }
    struct Scope {Snapshot* old;~Scope(){pending=old;}} scope{pending};pending=sample?&snapshot:nullptr;
    SetLastError(incoming);const auto result=request(owner,name,listener,offset,bytes,buffer,thread,priority,flag,user);
    const DWORD error=GetLastError();SetLastError(error);return result;
}
inline void Construct(U64 owner,void* record){
    if(!pending||pending->owner!=owner||Field(record,0x138)!=pending->texture||Field(record,0x140)!=pending->level)return;
    Snapshot snapshot=*pending;snapshot.id=Field(record,0x178);if(!snapshot.id){++dropped;return;}
    if(guard.test_and_set(std::memory_order_acquire)){++dropped;return;}
    {
        Unlock unlock;auto& e=entries[snapshot.id&255];auto& p=publications[PubIndex(snapshot.texture,snapshot.level)];
        if(e.occupied||p.occupied){++dropped;return;}
        e={snapshot,true,false,false};p={snapshot.texture,snapshot.id,owner,snapshot.level,true};++constructed;
    }
    Log("TEXTURE_R240_REQUEST owner=%llu id=%llu texture=%llu level=%u offset=%llu bytes=%llu container=%llu resource=%llu mip_state=%u resource_state=0x%x native_flags=0x%x heap=%u format=%u layout=%u mips=%u compact_bytes=%llu footprint_bytes=%llu direct_mips=%u tight_size_match=%u",
        owner,snapshot.id,snapshot.texture,snapshot.level,snapshot.offset,snapshot.bytes,snapshot.container,snapshot.resource,snapshot.mipState,snapshot.state,snapshot.nativeFlags,snapshot.heap,snapshot.format,unsigned(snapshot.layoutValid),snapshot.layout.count,snapshot.layout.compactBytes,snapshot.layout.footprintBytes,snapshot.layout.directMips,unsigned(snapshot.layoutValid&&snapshot.bytes==snapshot.layout.compactBytes));
    for(unsigned i=0;i<snapshot.layout.count;++i){const auto& m=snapshot.layout.mips[i];
        Log("TEXTURE_R240_MIP owner=%llu id=%llu mip=%u width=%u height=%u rows=%u row_bytes=%llu row_pitch=%u compact_offset=%llu compact_bytes=%llu footprint_bytes=%llu",owner,snapshot.id,i,m.width,m.height,m.rows,m.rowBytes,m.rowPitch,m.compactOffset,m.compactBytes,m.footprintBytes);
    }
}
inline void Read(HANDLE file,const loader_candidate::Candidate& actual,DWORD received,bool success){
    if(guard.test_and_set(std::memory_order_acquire))return;
    bool match=false;
    {
        Unlock unlock;auto& e=entries[actual.id&255];
        match=e.occupied&&e.data.id==actual.id&&e.data.owner==actual.owner&&!e.read;
        if(match){e.read=true;++reads;}
    }
    if(!match)return;
    BY_HANDLE_FILE_INFORMATION info{};const bool identity=GetFileInformationByHandle(file,&info)!=FALSE;
    Log("TEXTURE_R240_READ owner=%llu id=%llu pack=%llu archive_offset=%llu requested=%llu received=%lu success=%u identity=%u volume=%lu file_index=%llu file_size=%llu",
        actual.owner,actual.id,actual.pack,actual.range.offset,actual.range.bytes,received,unsigned(success),unsigned(identity),info.dwVolumeSerialNumber,(U64(info.nFileIndexHigh)<<32)|info.nFileIndexLow,(U64(info.nFileSizeHigh)<<32)|info.nFileSizeLow);
}
inline void UploadHook(void* file,unsigned mode,void* container){
    const DWORD incoming=GetLastError();U64 id=0,owner=0;
    if(!guard.test_and_set(std::memory_order_acquire)){
        Unlock unlock;
        Entry* found=nullptr;bool ambiguous=false;
        for(auto& e:entries)if(e.occupied&&e.data.container==reinterpret_cast<U64>(container)&&!e.upload){
            if(found){ambiguous=true;break;}found=&e;
        }
        if(ambiguous)++dropped;
        else if(found&&container){found->upload=true;id=found->data.id;owner=found->data.owner;++uploads;}
    }
    if(id)Log("TEXTURE_R240_UPLOAD owner=%llu id=%llu payload_mode=%u container=%llu native_upload=1",owner,id,mode,reinterpret_cast<U64>(container));
    SetLastError(incoming);upload(file,mode,container);
}
inline void PublishHook(void* owner,void* record){
    const DWORD incoming=GetLastError();const U64 texture=Field(record,0);const unsigned level=Dword(record,8);
    U64 id=0,loader=0;bool read=false,uploaded=false;
    if(!guard.test_and_set(std::memory_order_acquire)){
        Unlock unlock;auto& p=publications[PubIndex(texture,level)];
        if(p.occupied&&p.texture==texture&&p.level==level){
            auto& e=entries[p.id&255];
            if(e.occupied&&e.data.id==p.id&&e.data.owner==p.owner){id=p.id;loader=p.owner;read=e.read;uploaded=e.upload;e={};p={};++queued;}
        }
    }
    if(id){const bool success=*(static_cast<unsigned char*>(record)+12)!=0;
        Log("TEXTURE_R240_PUBLICATION_QUEUED owner=%llu id=%llu texture=%llu level=%u success=%u read_observed=%u upload_observed=%u gpu_ready_measured=0",loader,id,texture,level,unsigned(success),unsigned(read),unsigned(uploaded));}
    SetLastError(incoming);publish(owner,record);
}
inline bool Install(){
    char enabled[8]{};if(GetEnvironmentVariableA("CONTROLFG_TEXTURE_CONTRACT",enabled,sizeof(enabled))!=1||enabled[0]!='1')return false;
    if(!native_loader_trace::installed){Log("TEXTURE_R240 disabled=loader_hooks_required");return false;}
    auto renderer=GetModuleHandleW(L"renderer_rmdwin10_f.dll"),d3d=GetModuleHandleW(L"d3d_rmdwin10_f.dll"),runtime=GetModuleHandleW(L"rl_rmdwin10_f.dll");
    if(!renderer||!d3d||!runtime||!HashMatches(renderer,"EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433")||
        !HashMatches(d3d,"CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5")){
        Log("TEXTURE_R240 disabled=module_hash");return false;
    }
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    request=reinterpret_cast<Request>(reinterpret_cast<unsigned char*>(runtime)+0xfa2b0);
    upload=reinterpret_cast<Upload>(base+0x1546e0);publish=reinterpret_cast<Publish>(base+0x160370);
    if(!requestPatch.Prepare(base+0x158247,reinterpret_cast<void**>(base+0x5e0140),reinterpret_cast<void*>(request),reinterpret_cast<void*>(&RequestHook))){Log("TEXTURE_R240 disabled=request_prepare");return false;}
    const unsigned sites[]={0x15831a,0x158374,0x1583fe};
    for(unsigned i=0;i<3;++i)if(!NativeCpuPrepareCallPatch(base+sites[i],i?reinterpret_cast<void*>(publish):reinterpret_cast<void*>(upload),
        i?reinterpret_cast<void*>(&PublishHook):reinterpret_cast<void*>(&UploadHook),&callPatches[i])){Log("TEXTURE_R240 disabled=prepare index=%u",i);return false;}
    unsigned applied=0;
    for(;applied<3;){const auto i=applied;if(!NativeCpuExchangeCall(&callPatches[i],true))break;++applied;if(!callPatches[i].writeHealthy)break;}
    bool importApplied=false;if(applied==3&&callPatches[2].writeHealthy)importApplied=requestPatch.Exchange(true);
    if(applied!=3||!callPatches[2].writeHealthy||!importApplied||!requestPatch.healthy){
        if(importApplied&&!requestPatch.Exchange(false))Log("TEXTURE_R240 request_rollback_failed=1");
        while(applied){--applied;if(!NativeCpuExchangeCall(&callPatches[applied],false))Log("TEXTURE_R240 rollback_failed=%u",applied);}return false;
    }
    native_loader_stages::observeConstruct=&Construct;native_loader_stages::observeRead=&Read;installed=true;
    Log("TEXTURE_R240 installed=1 first=8 interval=64 cap=96 native_gpu_redirect=0 retained_native_pointers=0");return true;
}
inline void Poll(U64 present){
    static ULONGLONG next=0;if(!installed||present%60)return;const auto now=GetTickCount64();if(now<next)return;next=now+5000;
    Log("TEXTURE_R240_STATUS present=%llu requests=%llu captured=%llu constructed=%llu reads=%llu uploads=%llu publication_queued=%llu dropped=%llu invalid=%llu native_gpu_redirect=0",present,requests.load(),captured.load(),constructed.load(),reads.load(),uploads.load(),queued.load(),dropped.load(),invalid.load());
}
}
