#pragma once
#include "free_camera_control.h"

// Included inside native_lighting, after Trace. Installed during Configure,
// before returning the first DXGI factory; never patched from the overlay.
namespace free_camera {
inline control_free_camera::Commands commands;
inline std::atomic<bool> installed{false};
inline std::atomic<unsigned long long> updates{0};
inline std::atomic<DWORD> updateThread{0};
inline unsigned char* image = nullptr;
inline RRAlbedoCallPatch patch{};
using Update = void (*)();
using SetMode = void (*)(int);
using GetObject = void* (*)(void*);
using GetInputFlag = bool (*)(void*);
inline Update originalUpdate = nullptr;
inline SetMode setMode = nullptr;
inline GetObject getSession = nullptr, getAvatar = nullptr;
inline GetInputFlag getInputFlag = nullptr;

inline int ReadMode() noexcept { return *reinterpret_cast<const int*>(image + 0x129a5cc); }
inline void* Input() noexcept {
    // The IAT entry points to InputManager::sm_pInstance (an exported variable).
    return **reinterpret_cast<void***>(image + 0xd9c848);
}
inline bool GameplayReady() noexcept {
    // Mode 0 silently falls back to mode 1 without an avatar. Refuse that case,
    // and never activate in a title/loading scene with uninitialized cameras.
    __try {
        if (!Input()) return false;
        auto* client = *reinterpret_cast<unsigned char**>(image + 0x129a640);
        if (!client || !*reinterpret_cast<void**>(client + 0x30)) return false;
        auto* session = static_cast<unsigned char*>(getSession(*reinterpret_cast<void**>(client + 0x30)));
        if (!session) return false;
        void* controller = *reinterpret_cast<void**>(session + 0x6d0);
        if (!controller || !getAvatar(controller)) return false;
        auto** cameras = reinterpret_cast<void**>(image + 0x129a670);
        return cameras[0] && cameras[1] && *reinterpret_cast<void**>(image + 0x129a648);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool VerifyInput(int mode) noexcept { return getInputFlag(Input()) == (mode == 1); }
inline void UpdateHook() {
    // Preserve the native input/movement update exactly once, then service a
    // request before its caller consumes the selected camera transform.
    originalUpdate();
    if (!installed.load(std::memory_order_acquire)) return;
    const DWORD thread = GetCurrentThreadId();
    DWORD empty = 0;
    updateThread.compare_exchange_strong(empty, thread);
    if (updateThread.load() != thread) return; // do not service from another thread
    const bool pending = commands.Pending();
    const int before = commands.Mode();
    commands.Service(&ReadMode, &GameplayReady, [](int mode) { setMode(mode); }, &VerifyInput);
    const auto count = ++updates;
    if (pending || before != commands.Mode() || count == 1)
        Trace("free_camera_update mode=%d generation=%llu result=%u thread=%lu updates=%llu native_input_verified_on_apply=1",
            commands.Mode(),static_cast<unsigned long long>(commands.Generation()),unsigned(commands.Status()),thread,count);
}
inline const wchar_t* Status() noexcept {
    if (!installed.load()) return L"Camera unavailable";
    if (commands.Pending()) return L"Camera pending";
    switch (commands.Status()) {
    case control_free_camera::Result::NotGameplay: return L"Return to gameplay";
    case control_free_camera::Result::OtherMode: return L"Exit photo/cutscene";
    case control_free_camera::Result::VerificationFailed: return L"Camera check failed";
    case control_free_camera::Result::TimedOut: return L"Camera timed out";
    default: break;
    }
    return commands.Mode()==1 ? L"Free camera active" : commands.Mode()==0 ? L"Normal camera" : L"Waiting for game";
}
inline bool Install(HMODULE exe, bool verifiedSteamTarget) noexcept {
    if (installed.load()) return true;
    if (!exe || !verifiedSteamTarget) return false; // other storefront RVAs unverified
    auto* base = reinterpret_cast<unsigned char*>(exe);
    constexpr unsigned char prologue[] = {0x48,0x8b,0xc4,0x56,0x57,0x41,0x55,0x48,0x81,0xec,0x80,0,0,0,0x48,0x89,0x58,0x08};
    constexpr unsigned char callAndMode[] = {0xe8,0x02,0x0f,0,0,0x48,0x63,0x0d,0x27,0x69,0xf1,0};
    if (memcmp(base+0x384540,prologue,sizeof(prologue)) || memcmp(base+0x383c99,callAndMode,sizeof(callAndMode))) return false;
    const auto core = GetModuleHandleW(L"coregame_rmdwin10_f.dll");
    const auto input = GetModuleHandleW(L"input_rmdwin10_f.dll");
    if (!core || !input) return false;
    getSession = reinterpret_cast<GetObject>(GetProcAddress(core,"?getGameSession@GameClientBase@coregame@@QEAAPEAVClientGameSession@2@XZ"));
    getAvatar = reinterpret_cast<GetObject>(GetProcAddress(core,"?getAvatarObject@ClientPlayerController@coregame@@QEAAPEAVEntityState@2@XZ"));
    getInputFlag = reinterpret_cast<GetInputFlag>(GetProcAddress(input,"?isFreeCameraWithoutPlayerControlsOn@InputManager@input@@QEAA_NXZ"));
    const auto inputInstance = GetProcAddress(input,"?sm_pInstance@InputManager@input@@0PEAV12@EA");
    if (!getSession || !getAvatar || !getInputFlag || !inputInstance ||
        *reinterpret_cast<void**>(base+0xd9c230) != reinterpret_cast<void*>(getSession) ||
        *reinterpret_cast<void**>(base+0xd9b430) != reinterpret_cast<void*>(getAvatar) ||
        *reinterpret_cast<void**>(base+0xd9c918) != reinterpret_cast<void*>(getInputFlag) ||
        *reinterpret_cast<void**>(base+0xd9c848) != reinterpret_cast<void*>(inputInstance)) return false;
    image = base;
    setMode = reinterpret_cast<SetMode>(base+0x384540);
    originalUpdate = reinterpret_cast<Update>(base+0x384ba0);
    if (!RRAlbedoPrepareCallPatch(base+0x383c99,reinterpret_cast<void*>(originalUpdate),reinterpret_cast<void*>(&UpdateHook),&patch)) return false;
    if (!RRAlbedoExchangeCall(&patch,true)) return false;
    if (!patch.writeHealthy) {
        const bool restored=RRAlbedoExchangeCall(&patch,false);
        Trace("free_camera_install ready=0 rollback=%u",unsigned(restored && patch.writeHealthy));
        return false;
    }
    installed.store(true,std::memory_order_release);
    Trace("free_camera_install ready=1 build=R199 setter_rva=0x384540 update_call_rva=0x383c99 game_thread_requests=1 time_scale_unchanged=1");
    return true;
}
}
