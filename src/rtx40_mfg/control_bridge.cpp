// Explicit Control FG experiment. No DllMain initialization or global hooks.
#include "ada_patch.h"
#include "patches.h"
#include "provider_policy.h"
#include "log.h"
#include <mutex>
#include <string>
namespace {
std::mutex guard;
HMODULE provider = nullptr;
std::wstring providerPath;
bool started = false;
bool prepared = false;
bool failed = false;
unsigned attempts = 0;
}
// Must run before slSetD3DDevice, on the actual rendering device.
extern "C" __declspec(dllexport) unsigned int __cdecl ControlFGMFGInitialize(
    void* device, const wchar_t* runtime, const wchar_t* logs) noexcept {
    std::lock_guard lock(guard);
    if (started) return prepared ? 1u : 0u;
    started = true;
    mfglog::Open(logs);
    ada_patch::SetLogCallback(&mfglog::WriteMessage);
    if (!device || !runtime || !ada_patch::ObserveD3D12Device(device)) return 0;
    providerPath = std::wstring(runtime) + L"\\nvngx_dlssg.dll";
    provider = LoadLibraryExW(providerPath.c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    provider_policy::VersionTriplet v{};
    if (!provider || !provider_policy::IsSupportedProvider(provider, providerPath.c_str())
        || !provider_policy::ReadProviderVersion(providerPath.c_str(), v)
        || v.major != 310 || v.minor != 9 || v.build != 1) {
        mfglog::Write(L"Control test rejected: requires shipped DLSS-G 310.9.1");
        return 0;
    }
    const std::wstring pluginPath = std::wstring(runtime) + L"\\sl.dlss_g.dll";
    HMODULE plugin = GetModuleHandleW(pluginPath.c_str());
    if (!plugin) { mfglog::Write(L"Control test rejected: pinned plugin not loaded"); return 0; }
    // Pin both modules for process lifetime: patched descriptors must never dangle.
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        reinterpret_cast<LPCWSTR>(plugin), &pinned)) return 0;
    const auto arch = patches::PatchArchGates(provider, providerPath.c_str());
    const auto flip = patches::PatchFlipMetering(plugin, pluginPath.c_str());
    const auto maximum = patches::PatchStreamlineMaximum(plugin, pluginPath.c_str());
    prepared = arch.found >= 2 && arch.found == arch.patched
        && flip.located && flip.derived && flip.sites > 0
        && maximum.candidate && maximum.patched && maximum.compiledMaximum >= 3;
    mfglog::Write(L"CONTROL_MFG_PREPARE ready=%u arch=%zu/%zu flip=%zu maximum=%u; temporal pending",
        unsigned(prepared), arch.patched, arch.found, flip.sites, maximum.compiledMaximum);
    return prepared ? 1u : 0u;
}
// Runs before ANY FG creation, including 2x; never JIT the stock midpoint first.
extern "C" __declspec(dllexport) unsigned int __cdecl ControlFGMFGPrepare() noexcept {
    std::lock_guard lock(guard);
    if (!prepared || failed) return 0;
    if (ada_patch::Ready()) return ada_patch::PatchProvider(provider, providerPath.c_str()) ? 1u : 0u;
    // Retry only until initial provider descriptors are ready; cap repeated rejects.
    if (++attempts > 120) { failed = true; mfglog::Write(L"CONTROL_MFG_FAILED restart required"); return 0; }
    const bool ready = ada_patch::PatchProvider(provider, providerPath.c_str());
    if (ready || attempts == 1 || attempts == 120)
        mfglog::Write(L"CONTROL_MFG_TEMPORAL ready=%u attempt=%u failure=%u (%s)",
            unsigned(ready), attempts, ada_patch::FailureCode(), ada_patch::FailureName(ada_patch::FailureCode()));
    return ready ? 1u : 0u;
}
