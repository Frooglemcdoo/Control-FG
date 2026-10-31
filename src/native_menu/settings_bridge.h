#pragma once
#include <sstream>
#include <cstdio>
#include "../settings_catalog.h"

namespace control_native_menu {
using PropertyFn = void(*)(void*, int, int);
using ResetFn = void(*)(void*);
using ReadFn = void(*)(void*, void*, bool);
static PropertyFn originalEnum = nullptr, originalInt = nullptr;
static ResetFn originalReset = nullptr, originalSave = nullptr;
// Both initial configuration and live readback use the same catalog snapshot.
static std::string Configuration() {
    std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
    std::ostringstream output;
    output << "{\"rtx40\":" << (IsFGRtx40Series() ? "true" : "false");
    output << ",\"storageSupported\":" << (IsFGDirectStorageSupported() ? "true" : "false");
    auto appendValues = [&](const char* name, bool defaults) {
        output << ",\"" << name << "\":[";
        const auto values = control_settings::Snapshot(defaults);
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i) output << ',';
            output << values[i];
        }
        output << ']';
    };
    appendValues("values", false);
    appendValues("defaults", true);
    output << ",\"fgChoices\":[";
    bool comma = false;
    for (unsigned choice : control_settings::Definition(control_settings::Key::FrameGeneration).choices) {
        if (!control_settings::AllowedFG(choice)) continue;
        if (comma) output << ',';
        output << choice;
        comma = true;
    }
    output << "]}";
    return output.str();
}
static bool Apply(int id, int value) {
    const auto* option = control_settings::Find(id);
    if (!option || !control_settings::Write(*option, value)) return false;
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
    // Capabilities/defaults can change after the resource's startup snapshot.
    Emit(model, "CFG_CHOICES_BEGIN");
    for (unsigned choice : control_settings::Definition(control_settings::Key::FrameGeneration).choices) {
        if (!control_settings::AllowedFG(choice)) continue;
        char event[48]{};
        std::snprintf(event, sizeof(event), "CFG_CHOICE_FG_%u", choice);
        Emit(model, event);
    }
    Emit(model, "CFG_CHOICES_END");
    const auto defaults = control_settings::Snapshot(true);
    Emit(model, "CFG_DEFAULT_BEGIN");
    for (unsigned i = 0; i < defaults.size(); ++i) {
        for (unsigned bit = 0; bit < control_settings::kStateBits; ++bit) {
            if (!(defaults[i] & (1u << bit))) continue;
            char event[48]{};
            std::snprintf(event, sizeof(event), "CFG_DEFAULT_%u_%u", i, bit);
            Emit(model, event);
        }
    }
    Emit(model, "CFG_DEFAULT_END");
    const auto values = control_settings::Snapshot();
    Emit(model,"CFG_STATE_BEGIN");
    for(unsigned i=0;i<values.size();++i)for(unsigned bit=0;bit<control_settings::kStateBits;++bit)if(values[i]&(1u<<bit)) {
        char event[48]{};
        std::snprintf(event,sizeof(event),"CFG_STATE_%u_%u",i,bit);
        Emit(model,event);
    }
    Emit(model,"CFG_STATE_END");
}
static void Dispatch(void* model, int id, int value, PropertyFn original) {
    if(id==control_settings::kHandshakeId) { Log("NATIVE_MENU_HANDSHAKE version=%d",value);
        if (value == control_settings::kVersion && fgNativeMenuReady.load()) Sync(model);
        return; }
    if(id>=control_settings::kFirstId&&id<control_settings::kEndId) {
        std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
        if(id==control_settings::Definition(control_settings::Key::DirectStorage).id){
            const bool valid=fgNativeMenuReady.load()&&value>=0&&value<=1&&IsFGDirectStorageSupported();
            if(valid)originalSave(model);
            const bool ok=valid&&FGSettingsRestartStorage(static_cast<unsigned>(value));
            Notify(model,id,ok);
            Emit(model,ok?"CFG_STORAGE_RESTART_READY":"CFG_STORAGE_RESTART_FAILED");
            return;
        }
        const bool ok = fgNativeMenuReady.load() && Apply(id,value);
        Log("NATIVE_MENU_SETTING id=%d value=%d applied=%u",id,value,unsigned(ok));
        Notify(model,id,ok);
        if (ok && id == control_settings::Definition(control_settings::Key::ExperimentalMultiFrameGeneration).id) Sync(model);
        return; // Reserved IDs must never reach native renderer settings.
    }
    original(model,id,value);
}
static void EnumHook(void* model,int id,int value) { Dispatch(model,id,value,originalEnum); }
static void IntHook(void* model,int id,int value) { Dispatch(model,id,value,originalInt); }
static void ResetHook(void* model) {
    if(fgNativeMenuReady.load()) {
        std::lock_guard<std::recursive_mutex> lock(fgSettingsMutex);
        for (const auto& option : control_settings::kOptions) {
            if (!control_settings::Visible(option)) continue;
            const bool success = control_settings::Write(option, control_settings::Default(option),
                control_settings::WriteReason::Reset);
            if (success) FGSettingsMarkSettingsDirty();
            Notify(model, option.id, success);
        }
        FGSettingsFlushSettingsIfDue(true);
    }
    originalReset(model);
    if (fgNativeMenuReady.load()) Sync(model); // Show actual defaults/rejected resets, not an old UI guess.
}
static void SaveHook(void* model) { originalSave(model); FGSettingsFlushSettingsIfDue(true); }

}
