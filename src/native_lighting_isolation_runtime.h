#pragma once
#include "native_lighting_isolation.h"

namespace native_lighting {
enum class Mode { Native, DisableDirect, DisableBakedGi };

inline bool MarkerExists(const wchar_t* filename) noexcept {
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return false;
    const size_t prefix = static_cast<size_t>(slash - path) + 1;
    const size_t suffix = wcslen(filename) + 1;
    if (prefix + suffix > MAX_PATH) return false;
    memcpy(path + prefix, filename, suffix * sizeof(wchar_t));
    const DWORD attrs = GetFileAttributesW(path);
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

inline Mode ActiveMode() noexcept {
    static const Mode mode = []() noexcept {
        const bool direct = MarkerExists(L"ControlFG-Disable-Direct-R53.txt");
        const bool gi = MarkerExists(L"ControlFG-Disable-BakedGI-R53.txt");
        const Mode selected = direct == gi ? Mode::Native : direct ? Mode::DisableDirect : Mode::DisableBakedGi;
        Log("NATIVE_LIGHTING_R53 mode=%u marker_conflict=%u restart_required=1",
            unsigned(selected), unsigned(direct && gi));
        return selected;
    }();
    return mode;
}

inline bool Prepare(const ShaderPatch& patch, const D3D12_SHADER_BYTECODE& shader,
                    std::vector<unsigned char>& bytes) noexcept {
    if (!shader.pShaderBytecode || shader.BytecodeLength != patch.size) return false;
    char sha[65]{};
    if (!PTReflectionSha256(static_cast<const unsigned char*>(shader.pShaderBytecode), shader.BytecodeLength, sha) ||
        strcmp(sha, patch.nativeSha)) return false;
    try {
        char patchedSha[65]{};
        if (MakeDisabledContribution(patch, shader.pShaderBytecode, shader.BytecodeLength, sha, bytes) &&
            PTReflectionSha256(bytes.data(), bytes.size(), patchedSha) && !strcmp(patchedSha, patch.patchedSha)) return true;
    } catch (...) {
        bytes.clear();
    }
    Log("NATIVE_LIGHTING_R53 contribution=%s matched=1 applied=0 reason=patch_validation", patch.name);
    return false;
}

using CreateGraphicsPso = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, REFIID, void**);
inline CreateGraphicsPso originalGraphics = nullptr;
inline SRWLOCK graphicsInstallLock = SRWLOCK_INIT;

inline HRESULT STDMETHODCALLTYPE GraphicsHook(ID3D12Device* device,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, REFIID riid, void** out) {
    if (!originalGraphics) return E_UNEXPECTED;
    std::vector<unsigned char> bytes;
    if (ActiveMode() == Mode::DisableBakedGi && desc && out && Prepare(kBakedGi, desc->PS, bytes)) {
        auto replacement = *desc;
        replacement.PS = {bytes.data(), bytes.size()};
        replacement.CachedPSO = {nullptr, 0};
        void* candidate = nullptr;
        const HRESULT hr = originalGraphics(device, &replacement, riid, &candidate);
        Log("NATIVE_LIGHTING_R53 contribution=BakedGi matched=1 applied=%u hr=0x%08lX patched_sha=%s",
            unsigned(SUCCEEDED(hr) && candidate), static_cast<unsigned long>(hr), kBakedGi.patchedSha);
        if (SUCCEEDED(hr) && candidate) { *out = candidate; return hr; }
        if (candidate) reinterpret_cast<IUnknown*>(candidate)->Release();
    }
    return originalGraphics(device, desc, riid, out);
}

// Installed at device creation, before the game's native PSOs are created.
// A differing existing original on another vtable is left untouched.
inline bool InstallGraphicsHook(ID3D12Device* device) noexcept {
    if (ActiveMode() != Mode::DisableBakedGi) return true;
    if (!device) return false;
    void** vtable = nullptr;
    __try { vtable = *reinterpret_cast<void***>(device); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    if (!vtable) return false;
    auto** slot = &vtable[10]; // ID3D12Device::CreateGraphicsPipelineState
    AcquireSRWLockExclusive(&graphicsInstallLock);
    void* current = *slot;
    bool ready = current == reinterpret_cast<void*>(&GraphicsHook) && originalGraphics;
    if (!ready && current && (!originalGraphics || current == reinterpret_cast<void*>(originalGraphics))) {
        DWORD protection = 0;
        if (VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &protection)) {
            originalGraphics = reinterpret_cast<CreateGraphicsPso>(current);
            ready = InterlockedCompareExchangePointer(slot, reinterpret_cast<void*>(&GraphicsHook), current) == current;
            DWORD ignored = 0;
            const BOOL restored = VirtualProtect(slot, sizeof(void*), protection, &ignored);
            Log("NATIVE_LIGHTING_R53 graphics_hook=%u protection_restored=%u", unsigned(ready), unsigned(restored));
        }
    }
    ReleaseSRWLockExclusive(&graphicsInstallLock);
    if (!ready) Log("NATIVE_LIGHTING_R53 graphics_hook=0 reason=installation_failed native_fallback=1");
    return ready;
}
} // namespace native_lighting
