#pragma once
#include "native_lighting_isolation.h"
#include "native_lighting_live_state.h"

namespace native_lighting {
inline LiveSelection selection;
inline std::atomic<unsigned> readyMask{0};
inline SRWLOCK reportLock = SRWLOCK_INIT;
inline HANDLE reportFile = INVALID_HANDLE_VALUE;
inline std::atomic<bool> reportHealthy{false};
inline const GUID kVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x54,0x01}};
inline const GUID kKindGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x54,0x02}};

inline void Trace(const char* format, ...) noexcept {
    char detail[768]{};
    va_list args; va_start(args, format); vsnprintf_s(detail, sizeof(detail), _TRUNCATE, format, args); va_end(args);
    char line[960]{};
    const int length = snprintf(line, sizeof(line), "ms=%llu present=%llu %s\r\n",
        GetTickCount64(), presentCount.load(), detail);
    AcquireSRWLockExclusive(&reportLock);
    if (reportFile == INVALID_HANDLE_VALUE) {
        wchar_t path[MAX_PATH]{};
        const DWORD count = GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t* slash = count && count < MAX_PATH ? wcsrchr(path, L'\\') : nullptr;
        constexpr wchar_t filename[] = L"ControlFG-Lighting-R54.log";
        if (slash && size_t(slash-path)+1+_countof(filename) <= MAX_PATH) {
            memcpy(slash+1, filename, sizeof(filename));
            reportFile = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }
    }
    if (reportFile != INVALID_HANDLE_VALUE && length > 0) {
        DWORD written = 0;
        const DWORD expected = DWORD(length < int(sizeof(line)) ? length : sizeof(line)-1);
        const bool writtenOk = WriteFile(reportFile, line, expected, &written, nullptr) && written == expected;
        reportHealthy.store(writtenOk && FlushFileBuffers(reportFile));
    }
    ReleaseSRWLockExclusive(&reportLock);
    Log("NATIVE_LIGHTING_R54 %s", detail);
}

inline void SelectMode(Mode mode) noexcept {
    const unsigned required = mode == Mode::DisableBakedGi ? kGiBit : mode == Mode::DisableDirect ? kDirectBit : 0;
    if (required && !(readyMask.load() & required)) { Trace("selection_rejected mode=%s reason=variant_not_ready", ModeName(mode)); return; }
    const auto state = selection.Select(mode);
    Trace("selection seq=%llu mode=%s ready_mask=%u", LiveSelection::Sequence(state), ModeName(mode), readyMask.load());
}
inline void ReportVisual(bool specklesRemain) noexcept {
    const auto state = selection.Read();
    Trace("visual_result seq=%llu mode=%s result=%s recorded_mask=%u ready_mask=%u",
        LiveSelection::Sequence(state), ModeName(LiveSelection::GetMode(state)),
        specklesRemain ? "speckles_remain" : "speckles_gone", LiveSelection::Observed(state), readyMask.load());
}
inline bool Prepare(const ShaderPatch& patch, const D3D12_SHADER_BYTECODE& shader, std::vector<unsigned char>& bytes) noexcept {
    if (!shader.pShaderBytecode || shader.BytecodeLength != patch.size) return false;
    char sha[65]{}, patchedSha[65]{};
    if (!PTReflectionSha256(static_cast<const unsigned char*>(shader.pShaderBytecode), shader.BytecodeLength, sha) || strcmp(sha, patch.nativeSha)) return false;
    try {
        if (MakeDisabledContribution(patch, shader.pShaderBytecode, shader.BytecodeLength, sha, bytes) &&
            PTReflectionSha256(bytes.data(), bytes.size(), patchedSha) && !strcmp(patchedSha, patch.patchedSha)) return true;
    } catch (...) { bytes.clear(); }
    Trace("variant_failed contribution=%s reason=identity_or_allocation", patch.name);
    return false;
}
inline void AttachVariant(IUnknown* native, ID3D12PipelineState* variant, unsigned kind, HRESULT createHr) noexcept {
    HRESULT hr = createHr;
    ID3D12PipelineState* owner = nullptr;
    if (SUCCEEDED(hr) && variant && native && SUCCEEDED(native->QueryInterface(IID_PPV_ARGS(&owner)))) {
        hr = owner->SetPrivateDataInterface(kVariantGuid, variant);
        if (SUCCEEDED(hr)) hr = owner->SetPrivateData(kKindGuid, sizeof(kind), &kind);
        if (SUCCEEDED(hr)) readyMask.fetch_or(kind);
        else owner->SetPrivateDataInterface(kVariantGuid, nullptr);
    } else if (SUCCEEDED(hr)) hr = E_NOINTERFACE;
    if (owner) owner->Release();
    if (variant) variant->Release(); // Native PSO owns its alternate; no global PSO/device retention.
    Trace("variant contribution=%s ready=%u hr=0x%08lX", kind == kGiBit ? "BakedGi" : "Direct", unsigned(SUCCEEDED(hr)), static_cast<unsigned long>(hr));
}
using CreateCompute = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, REFIID, void**);
inline void ObserveCompute(ID3D12Device* device, const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc, IUnknown* native, CreateCompute original) noexcept {
    std::vector<unsigned char> bytes;
    if (!Prepare(kDirect, desc->CS, bytes)) return;
    auto replacement = *desc; replacement.CS = {bytes.data(), bytes.size()}; replacement.CachedPSO = {nullptr, 0};
    ID3D12PipelineState* variant = nullptr;
    const HRESULT hr = original(device, &replacement, IID_PPV_ARGS(&variant));
    AttachVariant(native, variant, kDirectBit, hr);
}

struct Binding {
    ID3D12PipelineState* selected;
    ID3D12PipelineState* alternate = nullptr;
    uint64_t state = selection.Read();
    unsigned kind = 0;
    explicit Binding(ID3D12PipelineState* native) noexcept : selected(native) {
        UINT size = sizeof(kind);
        if (!native || FAILED(native->GetPrivateData(kKindGuid, &size, &kind)) || size != sizeof(kind) || (kind != kGiBit && kind != kDirectBit)) { kind = 0; return; }
        if (Disables(LiveSelection::GetMode(state), kind)) {
            size = sizeof(alternate);
            if (SUCCEEDED(native->GetPrivateData(kVariantGuid, &size, &alternate)) && alternate) selected = alternate;
            else { kind = 0; Trace("bind_fallback reason=variant_missing"); }
        }
    }
    void Recorded() const noexcept {
        if (kind && selection.Observe(state, kind))
            Trace("binding_recorded seq=%llu mode=%s contribution=%s variant=%u", LiveSelection::Sequence(state),
                ModeName(LiveSelection::GetMode(state)), kind == kGiBit ? "BakedGi" : "Direct", unsigned(alternate != nullptr));
    }
    ~Binding() { if (alternate) alternate->Release(); }
    Binding(const Binding&) = delete;
    Binding& operator=(const Binding&) = delete;
};
using SetPso = void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*, ID3D12PipelineState*);
using ResetList = HRESULT (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*, ID3D12CommandAllocator*, ID3D12PipelineState*);
struct ListHooks { void** table = nullptr; SetPso set = nullptr; ResetList reset = nullptr; };
inline ListHooks listHooks[8]{};
inline SRWLOCK hookLock = SRWLOCK_INIT;
inline ListHooks Lookup(ID3D12GraphicsCommandList* list) noexcept {
    ListHooks result{};
    void** table = *reinterpret_cast<void***>(list);
    AcquireSRWLockShared(&hookLock);
    for (const auto& entry : listHooks) if (entry.table == table) { result = entry; break; }
    ReleaseSRWLockShared(&hookLock);
    return result;
}
inline void STDMETHODCALLTYPE SetHook(ID3D12GraphicsCommandList* list, ID3D12PipelineState* pso) {
    const auto original = Lookup(list).set;
    if (!original) return;
    Binding binding(pso); original(list, binding.selected); binding.Recorded();
}
inline HRESULT STDMETHODCALLTYPE ResetHook(ID3D12GraphicsCommandList* list, ID3D12CommandAllocator* allocator, ID3D12PipelineState* pso) {
    const auto original = Lookup(list).reset;
    if (!original) return E_UNEXPECTED;
    Binding binding(pso); const HRESULT hr = original(list, allocator, binding.selected);
    if (SUCCEEDED(hr)) binding.Recorded();
    return hr;
}
inline bool PatchSlot(void** slot, void* expected, void* hook) noexcept {
    DWORD old = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) return false;
    const bool installed = InterlockedCompareExchangePointer(slot, hook, expected) == expected;
    DWORD ignored = 0;
    if (!VirtualProtect(slot, sizeof(void*), old, &ignored)) Trace("hook_protection_restore_failed");
    return installed;
}
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
    void** table = *reinterpret_cast<void***>(list);
    AcquireSRWLockExclusive(&hookLock);
    ListHooks* available = nullptr;
    for (auto& entry : listHooks) {
        if (entry.table == table) { ReleaseSRWLockExclusive(&hookLock); return; }
        if (!entry.table && !available) available = &entry;
    }
    if (available) {
        *available = {table, reinterpret_cast<SetPso>(table[25]), reinterpret_cast<ResetList>(table[10])};
        const bool set = PatchSlot(&table[25], reinterpret_cast<void*>(available->set), reinterpret_cast<void*>(&SetHook));
        const bool reset = PatchSlot(&table[10], reinterpret_cast<void*>(available->reset), reinterpret_cast<void*>(&ResetHook));
        Trace("command_list_hooks set=%u reset=%u", unsigned(set), unsigned(reset));
    } else Trace("command_list_hooks failed=capacity");
    ReleaseSRWLockExclusive(&hookLock);
}
using CreateGraphics = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, REFIID, void**);
using CreateList = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, UINT, D3D12_COMMAND_LIST_TYPE, ID3D12CommandAllocator*, ID3D12PipelineState*, REFIID, void**);
inline CreateGraphics originalGraphics = nullptr;
inline CreateList originalCreateList = nullptr;
inline HRESULT STDMETHODCALLTYPE GraphicsHook(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, REFIID iid, void** out) {
    if (!originalGraphics) return E_UNEXPECTED;
    const HRESULT hr = originalGraphics(device, desc, iid, out);
    if (FAILED(hr) || !out || !*out || !desc) return hr;
    std::vector<unsigned char> bytes;
    if (Prepare(kBakedGi, desc->PS, bytes)) {
        auto replacement = *desc; replacement.PS = {bytes.data(), bytes.size()}; replacement.CachedPSO = {nullptr, 0};
        ID3D12PipelineState* variant = nullptr;
        const HRESULT variantHr = originalGraphics(device, &replacement, IID_PPV_ARGS(&variant));
        AttachVariant(reinterpret_cast<IUnknown*>(*out), variant, kGiBit, variantHr);
    }
    return hr; // The game always receives its original native PSO.
}
inline HRESULT STDMETHODCALLTYPE CreateListHook(ID3D12Device* device, UINT node, D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator* allocator, ID3D12PipelineState* pso, REFIID iid, void** out) {
    if (!originalCreateList) return E_UNEXPECTED;
    Binding binding(pso);
    const HRESULT hr = originalCreateList(device, node, type, allocator, binding.selected, iid, out);
    if (SUCCEEDED(hr) && out && *out) {
        ID3D12GraphicsCommandList* list = nullptr;
        if (SUCCEEDED(reinterpret_cast<IUnknown*>(*out)->QueryInterface(IID_PPV_ARGS(&list)))) { InstallList(list); list->Release(); }
        binding.Recorded();
    }
    return hr;
}
inline void StartOverlay() noexcept;
inline void InstallLiveDiagnostic(ID3D12Device* device) noexcept {
    if (!device) return;
    void** table = *reinterpret_cast<void***>(device);
    AcquireSRWLockExclusive(&hookLock);
    if (!originalGraphics) {
        originalGraphics = reinterpret_cast<CreateGraphics>(table[10]);
        if (!PatchSlot(&table[10], reinterpret_cast<void*>(originalGraphics), reinterpret_cast<void*>(&GraphicsHook))) originalGraphics = nullptr;
    }
    if (!originalCreateList) {
        originalCreateList = reinterpret_cast<CreateList>(table[12]);
        if (!PatchSlot(&table[12], reinterpret_cast<void*>(originalCreateList), reinterpret_cast<void*>(&CreateListHook))) originalCreateList = nullptr;
    }
    ReleaseSRWLockExclusive(&hookLock);
    StartOverlay();
}
} // namespace native_lighting
#ifndef CONTROL_LIGHTING_NO_OVERLAY
#include "ui/native_lighting_diagnostic.h"
#else
namespace native_lighting { inline void StartOverlay() noexcept {} }
#endif
