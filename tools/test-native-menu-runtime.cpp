// Tests the bridge and resource ownership with isolated renderer/settings stubs.
#include <atomic>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <cassert>
#include <iostream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
using DWORD=unsigned;using HMODULE=void*;
struct IMAGE_DOS_HEADER {unsigned e_magic;unsigned e_lfanew;};
struct IMAGE_NT_HEADERS64 {unsigned Signature;struct{unsigned TimeDateStamp;}FileHeader;struct{unsigned SizeOfImage;}OptionalHeader;};
constexpr unsigned IMAGE_DOS_SIGNATURE=0x5a4d,IMAGE_NT_SIGNATURE=0x4550,PAGE_READWRITE=4;
static HMODULE GetModuleHandleW(const wchar_t*){return nullptr;}
static bool VirtualProtect(void*,size_t,unsigned,unsigned*){return false;}
static void* InterlockedExchangePointer(void** p,void* v){auto old=*p;*p=v;return old;}
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif
static std::recursive_mutex fgSettingsMutex;
static std::atomic<bool> fgNativeMenuReady{true};
static std::atomic<unsigned> slRtx40MfgRequested{0};
static unsigned mode=4,target=0,texture=0,saveCount=0,dirtyCount=0,nativeCalls=0;
static bool vram=true,glass=true,rtx40=false,textureApply=true;
static void Log(const char*,...){}
static unsigned maxMultiplier=6;
static unsigned GetFGMaxSupportedMultiplier(){return maxMultiplier;}
static bool IsFGSelectionAllowedByGpuPolicy(unsigned v){return !rtx40||v==0||v==2;}
static bool IsFGDynamicCapabilityKnown(){return true;}
static bool IsFGDynamicMFGSupported(){return !rtx40;}
static unsigned GetFGDynamicManualTargetFps(){return target;}
static unsigned GetFGUserMultiplier(){return mode;}
static bool IsFGRtx40Series(){return rtx40;}
static bool PTReflectionUserRequested(){return glass;}
static unsigned GetFGTextureStreamingBudgetMs(){return texture;}
static bool IsFGVramMonitorEnabled(){return vram;}
static bool IsRTX40MFGRequested(){return slRtx40MfgRequested!=0;}
static void SetFGUserMultiplier(unsigned v){mode=v;}
static void SetFGDynamicManualTargetFps(unsigned v){target=v;}
static void PTReflectionUserRequest(bool v){glass=v;}
static bool ApplyTextureStreamingExperimental(unsigned){return textureApply;}
static void SetFGTextureStreamingBudgetMs(unsigned v){texture=v;}
static void SetFGVramMonitorEnabled(bool v){vram=v;}
static void FGSettingsMarkSettingsDirty(){++dirtyCount;}
static void FGSettingsFlushSettingsIfDue(bool){++saveCount;}
namespace control_rr {
constexpr unsigned RRPresetE=5,RRPresetF=6;
static bool rr=false;static unsigned preset=6,gi=4;
static bool RRUserRequested(){return rr;}
static unsigned RRUserPresetValue(){return preset;}
static unsigned GISampleRequested(){return gi;}
static void RRUserRequest(bool v){rr=v;}
static void RRUserSetPresetValue(unsigned v){preset=v;}
static void RRUserMarkPresetRestartRequired(bool){}
static void GISampleSelect(unsigned v){gi=v;}
}
namespace control_rr_clamp {
static unsigned value=60;
static unsigned Requested(){return value;}
static void Select(unsigned v){value=v;}
}
#include "../src/native_menu/runtime.h"
static void Native(void*,int,int){++nativeCalls;}
static void NativeReset(void*){++nativeCalls;}
static std::vector<std::string> emitted;
static void Capture(void*,const char* event){emitted.emplace_back(event);}
int main(int argc,char** argv){
    using namespace control_native_menu;
    if (argc == 3 && std::string(argv[1]) == "--config") {
        std::ofstream(argv[2]) << Configuration();
        return 0;
    }
    for (const auto& option : control_settings::kOptions) {
        assert(control_settings::Find(option.id) == &option);
        assert(!control_settings::Valid(option, -1));
        if (control_settings::Visible(option))
            assert(control_settings::Valid(option, control_settings::Default(option)));
    }
    assert(control_settings::Find(10998) == nullptr);
    assert(control_settings::Snapshot().size() == control_settings::kOptionCount);
    assert(control_settings::ReadStored(control_settings::Definition(control_settings::Key::DynamicTargetFps)) == 0);
    assert(control_settings::Read(control_settings::Key::DynamicTargetFps) == 144);
    void* viewTable[0x220/8]{};
    viewTable[0x218/8]=reinterpret_cast<void*>(&Capture);
    void** view=viewTable;
    alignas(8) unsigned char model[0xd8]{};
    void* viewPointer=&view;
    memcpy(model+0xd0,&viewPointer,sizeof(viewPointer));
    mode=6;target=1000;
    EnumHook(model,10999,41);
    assert(emitted.front()=="CFG_CHOICES_BEGIN"&&emitted.back()=="CFG_STATE_END");
    unsigned decoded[11]{};
    for(const auto& event:emitted){unsigned i,b;if(std::sscanf(event.c_str(),"CFG_STATE_%u_%u",&i,&b)==2){assert(i<11&&b<10);decoded[i]|=1u<<b;}}
    assert(decoded[0]==6&&decoded[1]==1&&decoded[2]==1000);
    assert(mode==6&&target==1000&&dirtyCount==0); // Readback never writes settings.
    mode=4;target=0;control_settings::manualTarget=144;
    originalEnum=originalInt=Native;originalReset=originalSave=NativeReset;
    EnumHook(nullptr,10003,1);assert(control_rr::rr && nativeCalls==0);
    IntHook(nullptr,10005,75);assert(control_rr_clamp::value==75);
    IntHook(nullptr,10005,76);assert(control_rr_clamp::value==75);
    EnumHook(nullptr,10998,1);assert(nativeCalls==0);
    EnumHook(nullptr,14,3);assert(nativeCalls==1);
    EnumHook(nullptr,10001,1);assert(target==0); // Disabled dependent setting is rejected natively.
    EnumHook(nullptr,10000,1);
    EnumHook(nullptr,10001,1);assert(target==144);
    IntHook(nullptr,10002,240);assert(target==240);
    EnumHook(nullptr,10001,0);assert(target==0);
    EnumHook(nullptr,10001,1);assert(target==240);
    textureApply=false;EnumHook(nullptr,10008,6);assert(texture==0);
    textureApply=true;EnumHook(nullptr,10008,6);assert(texture==6);
    rtx40=true;EnumHook(nullptr,10000,6);assert(mode==1);
    assert(control_settings::RestoreFG(0)==0);
    assert(control_settings::RestoreFG(6)==2);
    maxMultiplier=1;
    assert(control_settings::Default(control_settings::Key::FrameGeneration)==0);
    maxMultiplier=6;
    ResetHook(nullptr);assert(mode==2&&target==0&&glass&&texture==0&&!control_rr::rr);
    assert(control_rr::gi==4&&control_rr_clamp::value==60&&saveCount==1&&nativeCalls==2);
    SaveHook(nullptr);assert(saveCount==2&&nativeCalls==3);
    if(argc==2){
        rtx40=false;
        std::ifstream in(argv[1],std::ios::binary);
        std::string js{std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
        assert(js.size()==615195);
        alignas(8) unsigned char resource[0x1e0]{};
        FileEntry entries[2]{};
        const char* names[]{"uiresources/p7/commons.bundle.js","uiresources/p7/commons.css"};
        for(unsigned i=0;i<2;++i)memcpy(entries[i].path,&names[i],sizeof(char*));
        entries[0].size=js.size();entries[1].offset=js.size();entries[1].size=8;
        *reinterpret_cast<Bytes*>(resource+0x1c0)={reinterpret_cast<char*>(entries),2,2};
        auto* buffer=reinterpret_cast<Bytes*>(resource+0x1d0);
        buffer->size=buffer->flags=unsigned(js.size()+8);
        buffer->data=static_cast<char*>(malloc(buffer->size));
        memcpy(buffer->data,js.data(),js.size());memcpy(buffer->data+js.size(),"css-tail",8);
        reserveBytes=[](Bytes* b,size_t size){b->data=static_cast<char*>(realloc(b->data,size));b->flags=unsigned(size);};
        PatchResource(resource);
        assert(entries[0].size>js.size()&&entries[1].offset==entries[0].size);
        assert(memcmp(buffer->data+entries[1].offset,"css-tail",8)==0);
        assert(buffer->size==entries[0].size+8);
        const unsigned patchedSize=buffer->size;
        PatchResource(resource);assert(buffer->size==patchedSize); // Never patch twice.
        free(buffer->data);
    }
    rtx40 = true;
    for (const auto& option : control_settings::kOptions) {
        const unsigned expected = control_settings::Default(option);
        assert(control_settings::Write(option, expected, control_settings::WriteReason::Reset));
        assert(control_settings::Read(option.key) == expected);
    }
    assert(target == 0); // Resetting the slider never changes Automatic to Manual.
    std::cout<<"PASS native callback routing, validation, reset, save and resource relocation\n";
}
