#pragma once
#include "native_lighting_isolation.h"
#include "native_lighting_live_state.h"
#include "rr_lighting_diagnostic.h"
#include "native_lighting_report.h"
#include "native_lighting_log_queue.h"
#include "rt_shadow_capture_candidates.h"
#include "rt_spot_capture_bounds.h"
#include "rt_spot_table_coverage.h"
#include "spot_shadow_resolution_policy.h"
#include "rt_spot_culling_policy.h"
#include <intrin.h>
#include <string>

namespace native_lighting {
namespace rt_spot_capture_audit { inline void ReleaseFinishedOwners() noexcept; }
namespace rt_spot_table_audit { inline void Request() noexcept; }
inline LiveSelection selection;
inline std::atomic<unsigned> readyMask{0};
inline std::atomic<unsigned long long> graphicsSeen{0}, computeSeen{0}, bindingsSeen{0};
inline std::atomic<unsigned> candidateMismatches{0};
inline SRWLOCK reportLock = SRWLOCK_INIT;
inline SessionReport report;
inline DiagnosticLogQueue diagnosticLog;
inline std::atomic<bool> reportHealthy{false};
inline const GUID kVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x54,0x01}};
inline const GUID kDiffuseVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x56,0x01}};
inline const GUID kSpecularVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x56,0x02}};
inline const GUID kShadowVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x57,0x01}};
inline const GUID kScreenVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x58,0x01}};
inline const GUID kSpotVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x58,0x02}};
inline const GUID kPointVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x58,0x03}};
inline const GUID kSpotVarianceLowVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x59,0x01}};
inline const GUID kSpotVarianceHighVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x59,0x02}};
inline const GUID kSpotPcfCenterVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x59,0x03}};
inline const GUID kSpotPcf32VariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x59,0x04}};
inline const GUID kSpotStableSoftVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x60,0x01}};
inline const GUID kSpotStableTightVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x60,0x02}};
inline const GUID kSpotBilinearVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x61,0x01}};
inline const GUID kPatternVariantGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x57,0x02}};
inline const GUID& VariantGuid(Mode mode) noexcept {
    if (mode == Mode::DisableDirectDiffuse) return kDiffuseVariantGuid;
    if (mode == Mode::DisableDirectSpecular) return kSpecularVariantGuid;
    if (mode == Mode::BypassShadows) return kShadowVariantGuid;
    if (mode == Mode::BypassScreenShadows) return kScreenVariantGuid;
    if (mode == Mode::BypassSpotShadows) return kSpotVariantGuid;
    if (mode == Mode::BypassPointShadows) return kPointVariantGuid;
    if (mode == Mode::SpotVarianceLow) return kSpotVarianceLowVariantGuid;
    if (mode == Mode::SpotVarianceHigh) return kSpotVarianceHighVariantGuid;
    if (mode == Mode::SpotPcfCenter) return kSpotPcfCenterVariantGuid;
    if (mode == Mode::SpotPcf32) return kSpotPcf32VariantGuid;
    if (mode == Mode::SpotStableSoft) return kSpotStableSoftVariantGuid;
    if (mode == Mode::SpotStableTight) return kSpotStableTightVariantGuid;
    if (mode == Mode::SpotBilinear) return kSpotBilinearVariantGuid;
    if (mode == Mode::DisableLightPattern) return kPatternVariantGuid;
    return kVariantGuid;
}
inline const GUID kKindGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x54,0x02}};

inline void Trace(const char* format, ...) noexcept {
    char detail[768]{};
    va_list args; va_start(args, format); vsnprintf_s(detail, sizeof(detail), _TRUNCATE, format, args); va_end(args);
    char line[960]{};
    const int length = snprintf(line, sizeof(line), "ms=%llu present=%llu %s\r\n",
        GetTickCount64(), presentCount.load(), detail);
    if (length > 0) diagnosticLog.Push(line, size_t(length < int(sizeof(line)) ? length : sizeof(line)-1));
}
// UI-thread snapshots: totals are independent atomic observations, not one GPU frame.
inline void ReportRRPaths(const char* reason) noexcept {
    namespace diagnostic = control_rr_diagnostic;
    const auto state=selection.Read();
    const auto counts=diagnostic::counters.Snapshot();
    Trace("rr_path_snapshot reason=%s seq=%llu mode=%s rt_effects=0x%X rr_selected=%u last_result=0x%08X cpu_observations=1 gpu_completion_confirmed=0",
        reason,LiveSelection::Sequence(state),ModeName(LiveSelection::GetMode(state)),
        diagnostic::lastEffects.load(),unsigned(diagnostic::lastSelected.load()),diagnostic::lastEvaluationResult.load());
    for(std::size_t i=0; i<diagnostic::kCount; ++i)
        Trace("rr_path_count seq=%llu event=%s total=%llu",LiveSelection::Sequence(state),diagnostic::kEventNames[i],counts[i]);
}
// The diagnostic UI thread owns all report I/O; no disk lock is shared with
// render-side producers. Routine drains use buffered writes; SAVE LOG flushes.
inline bool DrainDiagnosticLog(bool flush = false) noexcept {
    rt_spot_capture_audit::ReleaseFinishedOwners();
    static char batch[DiagnosticLogQueue::Capacity()];
    AcquireSRWLockExclusive(&reportLock);
    const size_t length = diagnosticLog.Drain(batch,sizeof(batch));
    const auto dropped = diagnosticLog.TakeDropped();
    bool ok = report.OpenDefault();
    if (ok && length) ok = report.Write(batch,static_cast<DWORD>(length));
    if (ok && dropped) {
        char loss[128]{};
        const int size = snprintf(loss,sizeof(loss),"diagnostic_lines_dropped=%llu reason=queue_contention_or_capacity\r\n",dropped);
        ok = report.Write(loss,static_cast<DWORD>(size));
    }
    if (ok && flush) ok = report.Flush();
    reportHealthy.store(ok);
    ReleaseSRWLockExclusive(&reportLock);
    return ok;
}

inline const GUID kIdentityGuid = {0x822329d7,0x3614,0x4f73,{0xa6,0x14,0xd0,0x8c,0x94,0xf2,0x54,0x03}};
struct PipelineIdentity { unsigned stage = 0; unsigned logged = 0; unsigned long long bytes = 0; char sha[65]{}; };
inline SRWLOCK identityLock = SRWLOCK_INIT;
inline std::atomic<unsigned> identityReports{0};
inline void IdentifyPipeline(IUnknown* object, const D3D12_SHADER_BYTECODE& shader, unsigned stage) noexcept {
    if (!object || !shader.pShaderBytecode || !shader.BytecodeLength) return;
    ID3D12PipelineState* pso = nullptr;
    if (FAILED(object->QueryInterface(IID_PPV_ARGS(&pso)))) return;
    PipelineIdentity identity{}; UINT size = sizeof(identity);
    // Library loads can return an already-observed object. Keep its report flag.
    if (FAILED(pso->GetPrivateData(kIdentityGuid, &size, &identity)) || !identity.stage) {
        identity = {}; identity.stage = stage; identity.bytes = shader.BytecodeLength;
        if (PTReflectionSha256(static_cast<const unsigned char*>(shader.pShaderBytecode), shader.BytecodeLength, identity.sha))
            pso->SetPrivateData(kIdentityGuid, sizeof(identity), &identity);
    }
    pso->Release();
}
namespace rt_shadow_capture { inline void RecordComputeBinding(size_t, const char*) noexcept; }
inline void ReportBoundIdentity(ID3D12PipelineState* pso) noexcept {
    if (!pso) return;
    PipelineIdentity identity{}; UINT size = sizeof(identity);
    const HRESULT read = pso->GetPrivateData(kIdentityGuid, &size, &identity);
    if (SUCCEEDED(read) && identity.logged) return;
    AcquireSRWLockExclusive(&identityLock);
    size = sizeof(identity);
    pso->GetPrivateData(kIdentityGuid, &size, &identity);
    if (!identity.logged) {
        identity.logged = 1;
        if (identity.stage == 2) rt_shadow_capture::RecordComputeBinding(static_cast<size_t>(identity.bytes), identity.sha);
        pso->SetPrivateData(kIdentityGuid, sizeof(identity), &identity);
        if (identityReports.fetch_add(1) < 512)
            Trace("bound_pipeline pso=%p stage=%s bytes=%llu sha=%s", pso,
                identity.stage == 1 ? "PS" : identity.stage == 2 ? "CS" : "unobserved_creation",
                identity.bytes, identity.sha);
    }
    ReleaseSRWLockExclusive(&identityLock);
}
inline bool SelectMode(Mode mode) noexcept {
    if (!ModeAvailable(mode,readyMask.load())) { Trace("selection_rejected mode=%s reason=shader_or_sample_hook_not_ready", ModeName(mode)); return false; }
    ReportRRPaths("before_selection");
    const auto state = selection.Select(mode);
    if(mode==Mode::RoomBoth)rt_spot_table_audit::Request();
    Trace("selection seq=%llu mode=%s ready_mask=%u", LiveSelection::Sequence(state), ModeName(mode), readyMask.load());
    return true;
}
namespace spot_resolution_runtime { inline bool Observed(uint64_t state) noexcept; }
inline bool ReportVisual(bool specklesRemain) noexcept {
    const auto state = selection.Read();
    if (!CanReportVisual(LiveSelection::GetMode(state), LiveSelection::Observed(state)) ||
        (LiveSelection::GetMode(state) == Mode::SpotResolution2x && !spot_resolution_runtime::Observed(state))) {
        Trace("visual_result_rejected seq=%llu reason=awaiting_bind", LiveSelection::Sequence(state));
        return false;
    }
    Trace("visual_result seq=%llu mode=%s result=%s recorded_mask=%u ready_mask=%u",
        LiveSelection::Sequence(state), ModeName(LiveSelection::GetMode(state)),
        specklesRemain ? "speckles_remain" : "speckles_gone", LiveSelection::Observed(state), readyMask.load());
    return true;
}
inline bool Prepare(const ShaderPatch& patch, const D3D12_SHADER_BYTECODE& shader, std::vector<unsigned char>& bytes) noexcept {
    if (!shader.pShaderBytecode || shader.BytecodeLength != patch.size) return false;
    char sha[65]{}, patchedSha[65]{};
    if (!PTReflectionSha256(static_cast<const unsigned char*>(shader.pShaderBytecode), shader.BytecodeLength, sha)) {
        Trace("shader_hash_failed contribution=%s", patch.name); return false;
    }
    if (strcmp(sha, patch.nativeSha)) {
        if (candidateMismatches.fetch_add(1) < 12)
            Trace("shader_candidate_mismatch contribution=%s bytes=%llu actual=%s expected=%s", patch.name,
                static_cast<unsigned long long>(shader.BytecodeLength), sha, patch.nativeSha);
        return false;
    }
    try {
        if (MakeDisabledContribution(patch, shader.pShaderBytecode, shader.BytecodeLength, sha, bytes) &&
            PTReflectionSha256(bytes.data(), bytes.size(), patchedSha) && !strcmp(patchedSha, patch.patchedSha)) return true;
    } catch (...) { bytes.clear(); }
    Trace("variant_failed contribution=%s reason=identity_or_allocation", patch.name);
    return false;
}
inline void AttachVariant(IUnknown* native, ID3D12PipelineState* variant, unsigned kind, HRESULT createHr, const GUID& variantGuid = kVariantGuid, unsigned readyBit = 0) noexcept {
    HRESULT hr = createHr;
    ID3D12PipelineState* owner = nullptr;
    if (SUCCEEDED(hr) && variant && native && SUCCEEDED(native->QueryInterface(IID_PPV_ARGS(&owner)))) {
        hr = owner->SetPrivateDataInterface(variantGuid, variant);
        if (SUCCEEDED(hr)) hr = owner->SetPrivateData(kKindGuid, sizeof(kind), &kind);
        if (SUCCEEDED(hr)) readyMask.fetch_or(readyBit ? readyBit : kind);
        else owner->SetPrivateDataInterface(variantGuid, nullptr);
    } else if (SUCCEEDED(hr)) hr = E_NOINTERFACE;
    if (owner) owner->Release();
    if (variant) variant->Release(); // Native PSO owns its alternate; no global PSO/device retention.
    Trace("variant contribution=%s ready=%u hr=0x%08lX ready_bit=%u", kind == kGiBit ? "BakedGi" : "Direct", unsigned(SUCCEEDED(hr)), static_cast<unsigned long>(hr), readyBit ? readyBit : kind);
}
using CreateCompute = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, REFIID, void**);
namespace rt_spot_runtime {
inline void ObserveCompute(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, IUnknown*, CreateCompute) noexcept;
inline ID3D12PipelineState* SelectCompute(ID3D12PipelineState*) noexcept;
}
namespace rt_shadow_capture {
inline void RecordDirect(const D3D12_COMPUTE_PIPELINE_STATE_DESC*) noexcept;
inline void RecordCompute(const D3D12_COMPUTE_PIPELINE_STATE_DESC*) noexcept;
}
inline void ObserveCompute(ID3D12Device* device, const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc, IUnknown* native, CreateCompute original) noexcept {
    computeSeen.fetch_add(1);
    IdentifyPipeline(native, desc->CS, 2);
    rt_shadow_capture::RecordCompute(desc);
    rt_spot_runtime::ObserveCompute(device,desc,native,original);
    // Native shaders remain intact. Tag the captured direct-light shader solely
    // for the diagnostic's binding observation.
    if (desc->CS.BytecodeLength == kDirect.size && desc->CS.pShaderBytecode) {
        char sha[65]{};
        if (PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode), desc->CS.BytecodeLength, sha) &&
            !std::strcmp(sha, kDirect.nativeSha)) {
            rt_shadow_capture::RecordDirect(desc);
            ID3D12PipelineState* pso = nullptr;
            if (SUCCEEDED(native->QueryInterface(IID_PPV_ARGS(&pso)))) {
                const unsigned kind = kDirectBit;
                pso->SetPrivateData(kKindGuid, sizeof(kind), &kind);
                pso->Release();
            }
        }
    }
    (void)device; (void)original;
}

struct Binding {
    ID3D12PipelineState* selected;
    ID3D12PipelineState* alternate = nullptr;
    uint64_t state = selection.Read();
    unsigned kind = 0;
    explicit Binding(ID3D12PipelineState* native) noexcept : selected(native) {
        bindingsSeen.fetch_add(1);
        ReportBoundIdentity(native);
        UINT size = sizeof(kind);
        if (!native || FAILED(native->GetPrivateData(kKindGuid, &size, &kind)) || size != sizeof(kind) || (kind != kGiBit && kind != kDirectBit)) { kind = 0; return; }
        if (Disables(LiveSelection::GetMode(state), kind)) {
            size = sizeof(alternate);
            if (SUCCEEDED(native->GetPrivateData(VariantGuid(LiveSelection::GetMode(state)), &size, &alternate)) && alternate) selected = alternate;
            else { kind = 0; Trace("bind_fallback reason=variant_missing"); }
        }
    }
    void Recorded() const noexcept {
        if (kind && !IsRtSpotMode(LiveSelection::GetMode(state)) && selection.Observe(state, kind))
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
    Binding binding(pso);
    auto* alternate = rt_spot_runtime::SelectCompute(binding.selected);
    original(list, alternate ? alternate : binding.selected);
    if (alternate) alternate->Release();
    binding.Recorded();
}
inline HRESULT STDMETHODCALLTYPE ResetHook(ID3D12GraphicsCommandList* list, ID3D12CommandAllocator* allocator, ID3D12PipelineState* pso) {
    const auto original = Lookup(list).reset;
    if (!original) return E_UNEXPECTED;
    Binding binding(pso);
    auto* alternate = rt_spot_runtime::SelectCompute(binding.selected);
    const HRESULT hr = original(list, allocator, alternate ? alternate : binding.selected);
    if (alternate) alternate->Release();
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
#include "spot_shadow_resolution_runtime.h"
#include "rt_spot_capture_audit.h"
#include "rt_spot_table_audit.h"
#include "rt_spot_copy_audit.h"
#include "rt_shadow_capture.h"
#include "rt_spot_culling_runtime.h"
#include "rt_spot_runtime.h"
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
    rt_shadow_capture::InstallList(list);
    rt_spot_copy_audit::InstallList(list);
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
inline void ObserveGraphics(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, IUnknown* native) noexcept {
    graphicsSeen.fetch_add(1);
    IdentifyPipeline(native, desc->PS, 1);
    std::vector<unsigned char> bytes;
    if (Prepare(kBakedGi, desc->PS, bytes)) {
        auto replacement = *desc; replacement.PS = {bytes.data(), bytes.size()}; replacement.CachedPSO = {nullptr, 0};
        ID3D12PipelineState* variant = nullptr;
        const HRESULT variantHr = originalGraphics(device, &replacement, IID_PPV_ARGS(&variant));
        AttachVariant(native, variant, kGiBit, variantHr);
    }
}
inline HRESULT STDMETHODCALLTYPE GraphicsHook(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, REFIID iid, void** out) {
    if (!originalGraphics) return E_UNEXPECTED;
    const HRESULT hr = originalGraphics(device, desc, iid, out);
    if (SUCCEEDED(hr) && out && *out && desc) ObserveGraphics(device, desc, reinterpret_cast<IUnknown*>(*out));
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
#include "native_lighting_pipeline_library.h"
inline void StartOverlay() noexcept;
inline void InstallLiveDiagnostic(ID3D12Device* device) noexcept {
    if (!device) return;
    rt_spot_capture_audit::captured.store(true); // Retire speculative upload retention; R122 uses copy journals.
    rt_spot_table_audit::InstallQueues(device);
    rt_spot_runtime::Install();
    rt_shadow_capture::onRoot = &rt_spot_capture_audit::RememberRoot;
    rt_shadow_capture::replacementState = &rt_spot_runtime::CreateAugmented;
    rt_shadow_capture::onState = &rt_spot_runtime::OnState;
    rt_shadow_capture::replacementDispatch = &rt_spot_runtime::Dispatch;
    rt_shadow_capture::Install(device);
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
    Trace("device_hooks graphics=%u create_list=%u", unsigned(originalGraphics != nullptr), unsigned(originalCreateList != nullptr));
    InstallPipelineLibraries(device);
    StartOverlay();
}
} // namespace native_lighting
#ifndef CONTROL_LIGHTING_NO_OVERLAY
#include "ui/native_lighting_diagnostic.h"
#else
namespace native_lighting { inline void StartOverlay() noexcept {} }
#endif
