#pragma once
#include "pt_character_capture_bounds.h"
#include <sstream>
#include <iomanip>
#include <locale>
// R39 read-only, one-frame CPU upload capture. No GPU commands, shader changes,
// retained engine pointers, or reads after native unlock. Exact Steam hashes
// are checked by PTGlassRuntime::Install before this installer is called.
namespace PTCharacterCapture {
enum Kind : unsigned {Rigs,ObjectData,LocalToWorld,LocalToView,Skinning,PreviousSkinning,TLAS,Count};
static const char* const names[Count]={"rigs","object-data","local-to-world","local-to-view","skinning","previous-skinning","tlas"};
struct Record {
    unsigned kind=0,ordinal=0,thread=0;unsigned long long frame=0;
    std::uintptr_t buffer=0,view=0;std::uint64_t offset=0;
    std::vector<unsigned char> bytes;
};
struct Job {
    unsigned request=0;unsigned long long frame=0;
    unsigned rejected=0,counts[Count]{};std::size_t bytes=0;
    CameraSnapshot camera{};bool cameraValid=false;
    std::vector<Record> records;
};
static std::mutex captureMutex;
static Job job;
static std::atomic<unsigned long long> targetFrame{0};
static bool installed=false,keyHeld=false;
static unsigned requests=0;
using Unlock=void(*)(void*,void*);
using DynamicUnlock=void(*)(const void*,unsigned long long);
static Unlock nativeUnlock=nullptr;
static DynamicUnlock dynamicUnlock=nullptr;
static RRIndirectCallPatch patches[Count];
static bool Read(const void* source,void* dest,std::size_t bytes) noexcept {
    __try {if(!source)return false;memcpy(dest,source,bytes);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T> static bool Field(const void* base,std::size_t at,T& out) noexcept {
    const auto address=reinterpret_cast<std::uintptr_t>(base);
    if(!address||at>UINTPTR_MAX-address)return false;
    return Read(reinterpret_cast<const void*>(address+at),&out,sizeof(out));
}
static std::uintptr_t View() noexcept {
    __try {
        const auto base=reinterpret_cast<unsigned char*>(verifiedRenderer);
        const DWORD index=*reinterpret_cast<const DWORD*>(base+0x80283c);
        if(index>4095)return 0;
        const auto tls=reinterpret_cast<unsigned char**>(__readgsqword(0x58));
        return tls&&tls[index]?*reinterpret_cast<std::uintptr_t*>(tls[index]+8):0;
    }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
static unsigned long long ActiveFrame() noexcept {
    const auto target=targetFrame.load(std::memory_order_acquire);
    unsigned long long frame=0;DWORD fault=0;
    return target&&ReadEngineFrameSafe(&frame,&fault)&&frame==target?frame:0;
}
static void Copy(unsigned kind,unsigned long long frame,const void* data,std::size_t bytes,
                 const void* buffer,std::uint64_t offset) noexcept {
    try {
        std::lock_guard<std::mutex> guard(captureMutex);
        if(frame!=job.frame||targetFrame.load(std::memory_order_acquire)!=frame)return;
        if(!data||!bytes||bytes>MaxRecordBytes||job.counts[kind]>=4||
           bytes>MaxCaptureBytes-job.bytes){++job.rejected;return;}
        Record r;r.kind=kind;r.ordinal=job.counts[kind];r.frame=frame;
        r.thread=GetCurrentThreadId();r.buffer=reinterpret_cast<std::uintptr_t>(buffer);
        r.view=View();r.offset=offset;r.bytes.resize(bytes);
        if(!Read(data,r.bytes.data(),bytes)){++job.rejected;return;}
        job.records.push_back(std::move(r));++job.counts[kind];job.bytes+=bytes;
    }catch(...){Log("PT_CHARACTER_R39_COPY_FAILED kind=%u frame=%llu",kind,frame);}
}
static void CaptureMapped(unsigned kind,void* buffer,void* end) noexcept {
    const auto frame=ActiveFrame();if(!frame)return;
    void* mapping=nullptr;std::uintptr_t start=0;std::uint64_t length=0,offset=0;
    std::size_t bytes=0;
    const bool okay=Field(buffer,0x40,mapping)&&Field(mapping,0,length)&&
        Field(mapping,8,offset)&&Field(mapping,0x18,start)&&
        MappedBytes(start,length,reinterpret_cast<std::uintptr_t>(end),bytes);
    Copy(kind,frame,okay?reinterpret_cast<const void*>(start):nullptr,okay?bytes:0,buffer,offset);
}
template<unsigned K> static void UnlockHook(void* buffer,void* end) {
    CaptureMapped(K,buffer,end);nativeUnlock(buffer,end);
}
static void RigHook(const void* slot,unsigned long long allocation) {
    if(const auto frame=ActiveFrame()){
        std::uintptr_t start=0;std::uint32_t allocated=0,offset=0;
        const bool okay=Field(slot,0x50,start)&&Field(slot,0x58,offset)&&Field(slot,0x5c,allocated);
        const auto bytes=okay?RigBytes(allocated):0;
        Copy(Rigs,frame,start>=0x10000&&bytes<=UINTPTR_MAX-start?
             reinterpret_cast<const void*>(start):nullptr,bytes,slot,offset);
    }
    dynamicUnlock(slot,allocation);
}
static bool Write(const std::wstring& path,const void* data,std::size_t bytes) noexcept {
    HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD done=0;const bool good=bytes<=MAXDWORD&&WriteFile(file,data,static_cast<DWORD>(bytes),&done,nullptr)&&done==bytes;
    CloseHandle(file);return good;
}
static DWORD WINAPI Export(void* context) noexcept {
    std::unique_ptr<Job> capture(static_cast<Job*>(context));
    bool good=false;
    try {
        wchar_t env[32768]{};const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",env,32768);
        if(!length||length>=32768)throw 0;
        const auto root=std::wstring(env,length)+L"\\ControlFGProbe";
        if(!CreateDirectoryW(root.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)throw 0;
        const auto directory=root+L"\\character-r39-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(capture->frame);
        if(!CreateDirectoryW(directory.c_str(),nullptr))throw 0;
        std::ostringstream out;out.imbue(std::locale::classic());
        out<<"{\"schema\":\"control-character-r39\",\"pid\":"<<GetCurrentProcessId()
           <<",\"frame\":"<<capture->frame<<",\"request\":"<<capture->request
           <<",\"rejected\":"<<capture->rejected<<",\"rendering_changed\":false,\"records\":[";
        bool first=true;good=true;
        for(const auto& r:capture->records){
            const auto name=std::string(names[r.kind])+"-"+std::to_string(r.ordinal)+".bin";
            const bool saved=Write(directory+L"\\"+std::wstring(name.begin(),name.end()),r.bytes.data(),r.bytes.size());
            good=good&&saved;if(!first)out<<',';first=false;
            out<<"{\"file\":\""<<name<<"\",\"bytes\":"<<r.bytes.size()<<",\"kind\":"<<r.kind
               <<",\"thread\":"<<r.thread<<",\"view\":"<<r.view<<",\"buffer\":"<<r.buffer
               <<",\"offset\":"<<r.offset<<",\"saved\":"<<(saved?"true":"false")<<'}';
        }
        out<<"],\"camera_valid\":"<<(capture->cameraValid?"true":"false")
           <<",\"camera_view\":"<<reinterpret_cast<std::uintptr_t>(capture->camera.view)<<",\"view_to_world\":[";
        out<<std::setprecision(17);
        for(unsigned i=0;i<12;++i){if(i)out<<',';const double v=capture->camera.viewToWorld[i];if(std::isfinite(v))out<<v;else out<<"null";}
        out<<"],\"complete_files\":"<<(good?"true":"false")<<'}';
        const auto json=out.str();good=Write(directory+L"\\metadata.json",json.data(),json.size())&&good;
    }catch(...){good=false;}
    Log("PT_CHARACTER_R39_EXPORTED success=%u frame=%llu records=%zu rejected=%u bytes=%zu",
        unsigned(good),capture->frame,capture->records.size(),capture->rejected,capture->bytes);
    return 0;
}
static void Poll() noexcept {
    if(!installed)return;
    try {
        unsigned long long frame=0;DWORD fault=0;if(!ReadEngineFrameSafe(&frame,&fault)||!frame)return;
        const bool down=(GetAsyncKeyState(VK_F7)&0x8000)&&(GetAsyncKeyState(VK_CONTROL)&0x8000)&&(GetAsyncKeyState(VK_SHIFT)&0x8000);
        std::lock_guard<std::mutex> guard(captureMutex);
        const bool pressed=down&&!keyHeld;keyHeld=down;
        const auto target=targetFrame.load(std::memory_order_acquire);
        if(target&&frame==target){const char* reason=nullptr;job.cameraValid=ReadCamera(&job.camera,&fault,&reason)&&job.camera.engineFrame==target;}
        if(target&&frame>target+2){
            targetFrame.store(0,std::memory_order_release);
            auto* ready=new(std::nothrow) Job(std::move(job));job=Job{};
            if(ready&&!QueueUserWorkItem(&Export,ready,WT_EXECUTEDEFAULT)){delete ready;Log("PT_CHARACTER_R39_EXPORT_FAILED reason=worker");}
        }
        if(pressed&&!targetFrame.load()&&requests<2){
            DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
            if(foreground==GetCurrentProcessId()){
                job=Job{};job.request=++requests;job.frame=frame+2;
                targetFrame.store(job.frame,std::memory_order_release);
                Log("PT_CHARACTER_R39_REQUEST frame=%llu request=%u rendering_changed=0",job.frame,job.request);
            }
        }
    }catch(...){Log("PT_CHARACTER_R39_POLL_FAILED");}
}
static bool Install(HMODULE renderer,HMODULE d3d) noexcept {
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    auto* db=reinterpret_cast<unsigned char*>(d3d);
    nativeUnlock=reinterpret_cast<Unlock>(*reinterpret_cast<void**>(base+0x5dfe80));
    dynamicUnlock=reinterpret_cast<DynamicUnlock>(*reinterpret_cast<void**>(base+0x5dfda0));
    if(reinterpret_cast<void*>(nativeUnlock)!=db+0x26010||reinterpret_cast<void*>(dynamicUnlock)!=db+0x215c0)return false;
    const unsigned sites[Count]={0x314ae,0x124092,0x1240ab,0x1240bc,0x1240d5,0x1240e6,0xa8998};
    void* hooks[Count]={reinterpret_cast<void*>(&RigHook),reinterpret_cast<void*>(&UnlockHook<ObjectData>),
        reinterpret_cast<void*>(&UnlockHook<LocalToWorld>),reinterpret_cast<void*>(&UnlockHook<LocalToView>),
        reinterpret_cast<void*>(&UnlockHook<Skinning>),reinterpret_cast<void*>(&UnlockHook<PreviousSkinning>),reinterpret_cast<void*>(&UnlockHook<TLAS>)};
    for(unsigned i=0;i<Count;++i){
        const auto slot=i?0x5dfe80:0x5dfda0;
        if(!patches[i].Prepare(base+sites[i],reinterpret_cast<void**>(base+slot),
            *reinterpret_cast<void**>(base+slot),hooks[i]))return false;
    }
    unsigned applied=0;
    for(;applied<Count;++applied){
        if(!patches[applied].Exchange(true))break;
        if(!patches[applied].healthy){++applied;break;}
    }
    installed=applied==Count;
    for(unsigned i=0;i<applied;++i)installed=installed&&patches[i].healthy;
    if(!installed)while(applied){if(!patches[--applied].Exchange(false))Log("PT_CHARACTER_R39_ROLLBACK_FAILED index=%u",applied);}
    Log("PT_CHARACTER_R39_INSTALL ready=%u hotkey=CtrlShiftF7 max_requests=2 gpu_commands=0",unsigned(installed));
    return installed;
}
}
