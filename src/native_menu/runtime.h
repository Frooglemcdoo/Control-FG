#pragma once
#include <sstream>
#include <cstdio>
#include "bundle.h"

namespace control_native_menu {
using PropertyFn = void(*)(void*, int, int);
using ResetFn = void(*)(void*);
using ReadFn = void(*)(void*, void*, bool);
static PropertyFn originalEnum = nullptr, originalInt = nullptr;
static ResetFn originalReset = nullptr, originalSave = nullptr;
static ReadFn originalRead = nullptr;
static unsigned char* uiBase = nullptr;
static unsigned manualTarget = 144;

static bool AllowedFG(unsigned value) {
    const unsigned max = GetFGMaxSupportedMultiplier();
    return value <= 6 && IsFGSelectionAllowedByGpuPolicy(value) &&
        !(value == 1 && IsFGDynamicCapabilityKnown() && !IsFGDynamicMFGSupported()) &&
        !(value >= 2 && max && value > max);
}
static std::string Configuration() {
    std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
    const auto target = GetFGDynamicManualTargetFps();
    manualTarget = target ? target : 144;
    const unsigned defaultFG = AllowedFG(4) ? 4 : 2;
    std::ostringstream s;
    s << "{\"presetE\":" << control_rr::RRPresetE << ",\"presetF\":" << control_rr::RRPresetF
      << ",\"rtx40\":" << (IsFGRtx40Series() ? "true" : "false") << ",\"values\":["
      << GetFGUserMultiplier() << ',' << (target ? 1 : 0) << ',' << manualTarget << ','
      << unsigned(control_rr::RRUserRequested()) << ',' << control_rr::RRUserPresetValue() << ','
      << control_rr_clamp::Requested() << ',' << control_rr::GISampleRequested() << ','
      << unsigned(PTReflectionUserRequested()) << ',' << GetFGTextureStreamingBudgetMs() << ','
      << unsigned(IsFGVramMonitorEnabled()) << ',' << unsigned(IsRTX40MFGRequested()) << "],\"defaults\":["
      << defaultFG << ",0,144,0," << control_rr::RRPresetF << ",60,4,1,0,1,0],\"fgChoices\":[";
    bool comma = false;
    for (unsigned v=0; v<=6; ++v) if (AllowedFG(v)) { if(comma)s<<','; s<<v; comma=true; }
    s << "]}";
    return s.str();
}
static bool Apply(int id, int value) {
    if (value < 0) return false;
    const unsigned v = static_cast<unsigned>(value);
    switch (id) {
    case 10000: if(!AllowedFG(v))return false; SetFGUserMultiplier(v); break;
    case 10001: if(v>1)return false; SetFGDynamicManualTargetFps(v?manualTarget:0); break;
    case 10002: if(v<30||v>1000)return false; manualTarget=v; SetFGDynamicManualTargetFps(v); break;
    case 10003: if(v>1)return false; control_rr::RRUserMarkPresetRestartRequired(false); control_rr::RRUserRequest(v!=0); break;
    case 10004: if(v!=control_rr::RRPresetE&&v!=control_rr::RRPresetF)return false;
        control_rr::RRUserSetPresetValue(v); control_rr::RRUserMarkPresetRestartRequired(false); break;
    case 10005: if(v<25||v>75)return false; control_rr_clamp::Select(v); break;
    case 10006: if(v!=0&&v!=2&&v!=4&&v!=6&&v!=8)return false; control_rr::GISampleSelect(v); break;
    case 10007: if(v>1)return false; PTReflectionUserRequest(v!=0); break;
    case 10008: if(v!=0&&v!=4&&v!=6&&v!=8)return false;
        if(!ApplyTextureStreamingExperimental(v))return false;
        SetFGTextureStreamingBudgetMs(v); break;
    case 10009: if(v>1)return false; SetFGVramMonitorEnabled(v!=0); break;
    case 10010: if(v>1||!IsFGRtx40Series())return false; slRtx40MfgRequested.store(v); break;
    default: return false;
    }
    FGSettingsMarkSettingsDirty();
    return true;
}
static void Emit(void* model,const char* event) {
    if(!model)return;
    void* view=*reinterpret_cast<void**>(static_cast<unsigned char*>(model)+0xd0);
    if(!view)return;
    // Exact no-argument event path used by ui::Page::triggerEvent (ui RVA 0x29d0).
    auto** table=*reinterpret_cast<void***>(view);
    reinterpret_cast<void(*)(void*,const char*)>(table[0x218/8])(view,event);
}
static void Notify(void* model,int id,bool success) {
    char event[48]{};
    std::snprintf(event,sizeof(event),"CFG_%s_%d",success?"OK":"FAIL",id);
    Emit(model,event);
}
// Use the verified no-argument event ABI for live readback. Ten bits cover
// every menu value (including the 1000 FPS limit) without payload ABI guesses.
static void Sync(void* model) {
    std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
    const auto target=GetFGDynamicManualTargetFps();
    if(target)manualTarget=target;
    const unsigned values[]{GetFGUserMultiplier(),target?1u:0u,manualTarget,
        unsigned(control_rr::RRUserRequested()),control_rr::RRUserPresetValue(),
        control_rr_clamp::Requested(),control_rr::GISampleRequested(),
        unsigned(PTReflectionUserRequested()),GetFGTextureStreamingBudgetMs(),
        unsigned(IsFGVramMonitorEnabled()),unsigned(IsRTX40MFGRequested())};
    Emit(model,"CFG_STATE_BEGIN");
    for(unsigned i=0;i<11;++i)for(unsigned bit=0;bit<10;++bit)if(values[i]&(1u<<bit)) {
        char event[48]{};
        std::snprintf(event,sizeof(event),"CFG_STATE_%u_%u",i,bit);
        Emit(model,event);
    }
    Emit(model,"CFG_STATE_END");
}
static void Dispatch(void* model, int id, int value, PropertyFn original) {
    if(id==10999) { Log("NATIVE_MENU_HANDSHAKE version=%d",value); Sync(model); return; }
    if(id>=10000&&id<11000) {
        std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
        const bool ok = fgNativeMenuReady.load() && Apply(id,value);
        Log("NATIVE_MENU_SETTING id=%d value=%d applied=%u",id,value,unsigned(ok));
        Notify(model,id,ok);
        return; // Reserved IDs must never reach native renderer settings.
    }
    original(model,id,value);
}
static void EnumHook(void* model,int id,int value) { Dispatch(model,id,value,originalEnum); }
static void IntHook(void* model,int id,int value) { Dispatch(model,id,value,originalInt); }
static void ResetHook(void* model) {
    if(fgNativeMenuReady.load()) {
        std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
        manualTarget=144;
        const int defaults[]{AllowedFG(4)?4:2,0,144,0,int(control_rr::RRPresetF),60,4,1,0,1,0};
        for(int i=0;i<11;++i)if(i!=10||IsFGRtx40Series()) {
            const bool ok=i==2||Apply(10000+i,defaults[i]);
            Notify(model,10000+i,ok);
        }
        FGSettingsFlushSettingsIfDue(true);
    }
    originalReset(model);
}
static void SaveHook(void* model) { originalSave(model); FGSettingsFlushSettingsIfDue(true); }

struct Bytes { char* data; unsigned size, flags; };
using ReserveFn=void(*)(Bytes*,size_t);
static ReserveFn reserveBytes=nullptr;
struct FileEntry {
    unsigned char path[0x120];
    std::uint64_t resourceID, offset, size;
};
static_assert(sizeof(FileEntry)==0x138, "Verified UI file entry layout");
static void PatchResource(void* object) {
    auto* base=static_cast<unsigned char*>(object);
    auto* files=reinterpret_cast<Bytes*>(base+0x1c0);
    auto* bytes=reinterpret_cast<Bytes*>(base+0x1d0);
    if(!files->data||!bytes->data||files->size>4096||bytes->size>32*1024*1024)return;
    auto* entries=reinterpret_cast<FileEntry*>(files->data);
    FileEntry* target=nullptr;
    for(unsigned i=0;i<files->size;++i) {
        auto& f=entries[i];
        if(f.offset>bytes->size||f.size>bytes->size-f.offset)return;
        const char* path=*reinterpret_cast<const char* const*>(f.path);
        if(path && strcmp(path,"uiresources/p7/commons.bundle.js")==0)target=&f;
    }
    if(!target)return;
    std::string result;
    const std::string input(bytes->data+target->offset,static_cast<size_t>(target->size));
    if(!PatchCommons(input,Configuration(),result)) {
        Log("NATIVE_MENU_RESOURCE skipped=unsupported_bundle size=%llu",target->size); return;
    }
    // Build the entire replacement before touching engine-owned storage.
    std::string replacement(bytes->data,bytes->size);
    replacement.replace(static_cast<size_t>(target->offset),static_cast<size_t>(target->size),result);
    const auto oldEnd=target->offset+target->size;
    const auto growth=result.size()-static_cast<size_t>(target->size);
    for(unsigned i=0;i<files->size;++i) {
        const auto& f=entries[i];
        if(&f!=target && f.size && f.offset<oldEnd && f.offset+f.size>target->offset)return;
    }
    // Use the game's byte-vector reserve to preserve allocator/destructor ownership.
    reserveBytes(bytes,replacement.size());
    if(!bytes->data || (bytes->flags&0x3fffffffu)<replacement.size())return;
    memcpy(bytes->data,replacement.data(),replacement.size());
    bytes->size=static_cast<unsigned>(replacement.size());
    for(unsigned i=0;i<files->size;++i)if(&entries[i]!=target && entries[i].size && entries[i].offset>=oldEnd)entries[i].offset+=growth;
    target->size=result.size();
    fgNativeMenuReady.store(true);
    Log("NATIVE_MENU_RESOURCE ready=1 controls=native_options_tab bytes=%u popup=removed",bytes->size);
}
static void ReadHook(void* object,void* stream,bool flag) {
    originalRead(object,stream,flag);
    try { PatchResource(object); } catch(...) { Log("NATIVE_MENU_RESOURCE failed=exception"); }
}
static bool MatchModule(HMODULE module,DWORD stamp,DWORD size) {
    if(!module)return false;
    auto* b=reinterpret_cast<unsigned char*>(module);
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(b);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
    auto* pe=reinterpret_cast<IMAGE_NT_HEADERS64*>(b+dos->e_lfanew);
    return pe->Signature==IMAGE_NT_SIGNATURE && pe->FileHeader.TimeDateStamp==stamp && pe->OptionalHeader.SizeOfImage==size;
}
static bool Install() {
    HMODULE ui=GetModuleHandleW(L"ui_rmdwin10_f.dll"), core=GetModuleHandleW(L"coregame_rmdwin10_f.dll");
    if(!MatchModule(ui,0x67e06a9f,0x158000)||!MatchModule(core,0x67e06af5,0x928000)||
       !MatchModule(GetModuleHandleW(L"CoherentUIGT.dll"),0x5e8f6e9a,0x329000)) {
        Log("NATIVE_MENU_INSTALL ready=0 reason=unsupported_modules fallback=settings_file"); return false;
    }
    uiBase=reinterpret_cast<unsigned char*>(ui);
    reserveBytes=reinterpret_cast<ReserveFn>(uiBase+0x28f70);
    auto* c=reinterpret_cast<unsigned char*>(core);
    originalEnum=reinterpret_cast<PropertyFn>(c+0x1c9940);
    originalInt=reinterpret_cast<PropertyFn>(c+0x1c9830);
    originalReset=reinterpret_cast<ResetFn>(c+0x1ceb60);
    originalSave=reinterpret_cast<ResetFn>(c+0x1d5b60);
    originalRead=reinterpret_cast<ReadFn>(uiBase+0xc860);
    struct Patch { void** slot; void* expected; void* hook; DWORD protection; };
    Patch patches[]{
        {reinterpret_cast<void**>(c+0x5fb858+0x58),reinterpret_cast<void*>(originalEnum),reinterpret_cast<void*>(&EnumHook),0},
        {reinterpret_cast<void**>(c+0x5fb858+0x48),reinterpret_cast<void*>(originalInt),reinterpret_cast<void*>(&IntHook),0},
        {reinterpret_cast<void**>(c+0x5fb858+0x28),reinterpret_cast<void*>(originalReset),reinterpret_cast<void*>(&ResetHook),0},
        {reinterpret_cast<void**>(c+0x5fb858+0x18),reinterpret_cast<void*>(originalSave),reinterpret_cast<void*>(&SaveHook),0},
        {reinterpret_cast<void**>(uiBase+0xf71b8+0x130),reinterpret_cast<void*>(originalRead),reinterpret_cast<void*>(&ReadHook),0}
    };
    for(auto& p:patches)if(*p.slot!=p.expected){Log("NATIVE_MENU_INSTALL ready=0 reason=target_mismatch");return false;}
    // Sequential protection changes are restored immediately (several slots share a page).
    unsigned applied=0;
    for(auto& p:patches) {
        if(!VirtualProtect(p.slot,sizeof(void*),PAGE_READWRITE,&p.protection))break;
        InterlockedExchangePointer(p.slot,p.hook);
        DWORD ignored=0; VirtualProtect(p.slot,sizeof(void*),p.protection,&ignored);
        ++applied;
    }
    if(applied!=_countof(patches)) {
        while(applied) { auto& p=patches[--applied]; DWORD old=0,ignored=0;
            if(VirtualProtect(p.slot,sizeof(void*),PAGE_READWRITE,&old)){InterlockedExchangePointer(p.slot,p.expected);VirtualProtect(p.slot,sizeof(void*),old,&ignored);} }
        Log("NATIVE_MENU_INSTALL ready=0 reason=protect_failed"); return false;
    }
    Log("NATIVE_MENU_INSTALL ready=1 version=41 resource_patch=load_time_only"); return true;
}
}
