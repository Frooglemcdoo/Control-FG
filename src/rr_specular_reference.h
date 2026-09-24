#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "../diagnostic_addon/reference_clamp_contract.h"

namespace control_rr_reference {
inline constexpr wchar_t kAddonModuleName[] = L"ControlFG-RenoDXClampRef.addon64";
using namespace controlfg_clamp_ref_contract;

using StatusFn = uint32_t (*)();
using CountFn = uint64_t (*)();
using BuildIdFn = const char* (*)();
using U32Fn = uint32_t (*)();

struct Snapshot {
    HMODULE module = nullptr;
    uint32_t status = 0;
    uint64_t requests = 0;
    uint64_t confirms = 0;
    uint32_t targetCrc = 0;
    uint32_t replacementCrc = 0;
    bool exports = false;
    bool buildId = false;
    bool ready = false;
};

inline Snapshot Query() noexcept {
    Snapshot out{};
    out.module = GetModuleHandleW(kAddonModuleName);
    if (!out.module) return out;
    const auto statusFn = reinterpret_cast<StatusFn>(GetProcAddress(out.module,"ControlFGClampRef_Status"));
    const auto requestFn = reinterpret_cast<CountFn>(GetProcAddress(out.module,"ControlFGClampRef_ReplacementRequests"));
    const auto confirmFn = reinterpret_cast<CountFn>(GetProcAddress(out.module,"ControlFGClampRef_ReplacementConfirms"));
    const auto buildFn = reinterpret_cast<BuildIdFn>(GetProcAddress(out.module,"ControlFGClampRef_BuildId"));
    const auto targetFn = reinterpret_cast<U32Fn>(GetProcAddress(out.module,"ControlFGClampRef_TargetCRC"));
    const auto replacementFn = reinterpret_cast<U32Fn>(GetProcAddress(out.module,"ControlFGClampRef_ReplacementCRC"));
    if (!statusFn || !requestFn || !confirmFn || !buildFn || !targetFn || !replacementFn) return out;
    out.exports = true;
    __try {
        const char* id = buildFn();
        out.buildId = id && std::strcmp(id,BuildId)==0;
        out.status = statusFn();
        out.requests = requestFn();
        out.confirms = confirmFn();
        out.targetCrc = targetFn();
        out.replacementCrc = replacementFn();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        out = {};
        return out;
    }
    out.ready = Ready(out.buildId,out.status,out.requests,out.confirms,out.targetCrc,out.replacementCrc);
    return out;
}

inline bool Ready() noexcept { return Query().ready; }
}
