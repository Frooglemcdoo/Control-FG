#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "reshade_minimal_api18.h"
#include "reference_clamp_contract.h"
#include "controlfg_renodx_clamp_ref_shader.h"

static_assert(sizeof(void*) == 8, "ControlFG ClampRef is x64-only.");
static_assert(sizeof(reshade_min::shader_desc) == 48, "Pinned ReShade API 18 shader_desc ABI changed.");
static_assert(sizeof(reshade_min::pipeline_subobject) == 16, "Pinned ReShade API 18 pipeline_subobject ABI changed.");
static_assert(sizeof(reshade_min::pipeline_layout) == 8 && sizeof(reshade_min::pipeline) == 8, "Pinned ReShade handle ABI changed.");
static_assert(reshade_min::kApiVersion == 18 && reshade_min::kEventInitPipeline == 26 && reshade_min::kEventCreatePipeline == 27, "Pinned ReShade event ABI changed.");
static_assert(static_cast<uint32_t>(reshade_min::pipeline_subobject_type::compute_shader) == 6, "Pinned ReShade compute subobject value changed.");

namespace {
using namespace controlfg_clamp_ref_contract;

HMODULE gSelf = nullptr;
reshade_min::Api gApi{};
std::atomic<uint32_t> gStatus{0};
std::atomic<uint64_t> gReplacementRequests{0};
std::atomic<uint64_t> gReplacementConfirms{0};

uint32_t CRC32(const void* data, size_t size) noexcept {
    uint32_t crc = 0xFFFFFFFFu;
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) {
        crc ^= p[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc ^ 0xFFFFFFFFu;
}

void Log(int level, const char* text) noexcept {
    if (gApi.logMessage) gApi.logMessage(gSelf, level, text);
}

bool OnCreatePipeline(
    reshade_min::device*, reshade_min::pipeline_layout,
    uint32_t subobjectCount, const reshade_min::pipeline_subobject* subobjects) noexcept {
    if (!subobjects) return false;
    bool changed = false;
    for (uint32_t i = 0; i < subobjectCount; ++i) {
        if (subobjects[i].type != reshade_min::pipeline_subobject_type::compute_shader ||
            subobjects[i].count == 0 || !subobjects[i].data) continue;
        auto* desc = static_cast<reshade_min::shader_desc*>(subobjects[i].data);
        if (!desc->code || !desc->code_size) continue;
        const uint32_t crc = CRC32(desc->code, desc->code_size);
        if (crc != TargetShaderCRC) continue;
        gStatus.fetch_or(TargetSeen, std::memory_order_acq_rel);
        desc->code = kControlFGRenoDXClampRefShader;
        desc->code_size = sizeof(kControlFGRenoDXClampRefShader);
        desc->entry_point = nullptr;
        gReplacementRequests.fetch_add(1, std::memory_order_relaxed);
        gStatus.fetch_or(ReplacementRequested, std::memory_order_acq_rel);
        changed = true;
        Log(3, "ControlFG ClampRef: replaced Control compute shader 0x600347E7 with the RenoDX current-frame energy-clamp reference shader.");
    }
    return changed;
}

void OnInitPipeline(
    reshade_min::device*, reshade_min::pipeline_layout,
    uint32_t subobjectCount, const reshade_min::pipeline_subobject* subobjects,
    reshade_min::pipeline) noexcept {
    if (!subobjects) return;
    const uint32_t replacementCrc = CRC32(kControlFGRenoDXClampRefShader, sizeof(kControlFGRenoDXClampRefShader));
    for (uint32_t i = 0; i < subobjectCount; ++i) {
        if (subobjects[i].type != reshade_min::pipeline_subobject_type::compute_shader ||
            subobjects[i].count == 0 || !subobjects[i].data) continue;
        const auto* desc = static_cast<const reshade_min::shader_desc*>(subobjects[i].data);
        if (!desc->code || !desc->code_size) continue;
        const uint32_t crc = CRC32(desc->code, desc->code_size);
        if (crc == TargetShaderCRC) {
            gStatus.fetch_or(TargetSeen | TargetInitializedUnreplaced, std::memory_order_acq_rel);
            Log(2, "ControlFG ClampRef: target 0x600347E7 initialized without replacement; create_pipeline interception did not replace this PSO (possible pipeline-library path). Reference arm remains disabled.");
            continue;
        }
        if (crc != replacementCrc) continue;
        gReplacementConfirms.fetch_add(1, std::memory_order_relaxed);
        const uint32_t previous = gStatus.fetch_or(ReplacementConfirmed, std::memory_order_acq_rel);
        if ((previous & ReplacementConfirmed) == 0)
            Log(3, "ControlFG ClampRef: replacement pipeline confirmed by ReShade init_pipeline event; reference arm is ready.");
        return;
    }
}
}

extern "C" __declspec(dllexport) const char* NAME = "Control FG - RenoDX Clamp Reference";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "Diagnostic add-on: replaces only Control shader 0x600347E7 with the current-frame RenoDX-style specular energy clamp.";
extern "C" __declspec(dllexport) uint32_t ControlFGClampRef_Status() noexcept { return gStatus.load(std::memory_order_acquire); }
extern "C" __declspec(dllexport) uint64_t ControlFGClampRef_ReplacementRequests() noexcept { return gReplacementRequests.load(std::memory_order_acquire); }
extern "C" __declspec(dllexport) uint64_t ControlFGClampRef_ReplacementConfirms() noexcept { return gReplacementConfirms.load(std::memory_order_acquire); }
extern "C" __declspec(dllexport) const char* ControlFGClampRef_BuildId() noexcept { return BuildId; }
extern "C" __declspec(dllexport) uint32_t ControlFGClampRef_TargetCRC() noexcept { return TargetShaderCRC; }
extern "C" __declspec(dllexport) uint32_t ControlFGClampRef_ReplacementCRC() noexcept {
    return CRC32(kControlFGRenoDXClampRefShader, sizeof(kControlFGRenoDXClampRefShader));
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        gSelf = module;
        DisableThreadLibraryCalls(module);
        if (CRC32(kControlFGRenoDXClampRefShader, sizeof(kControlFGRenoDXClampRefShader)) != kControlFGRenoDXClampRefShaderCRC32) return FALSE;
        gApi = reshade_min::FindApi();
        if (!gApi.valid()) return FALSE;
        if (!gApi.registerAddon(module, reshade_min::kApiVersion)) return FALSE;
        gStatus.store(Registered | ShaderReady, std::memory_order_release);
        gApi.registerEventForAddon(module, reshade_min::kEventCreatePipeline, reinterpret_cast<void*>(&OnCreatePipeline));
        gApi.registerEventForAddon(module, reshade_min::kEventInitPipeline, reinterpret_cast<void*>(&OnInitPipeline));
        Log(3, "ControlFG ClampRef: registered with ReShade API 18; waiting for Control shader 0x600347E7.");
    } else if (reason == DLL_PROCESS_DETACH) {
        if (gApi.valid()) {
            gApi.unregisterEventForAddon(module, reshade_min::kEventCreatePipeline, reinterpret_cast<void*>(&OnCreatePipeline));
            gApi.unregisterEventForAddon(module, reshade_min::kEventInitPipeline, reinterpret_cast<void*>(&OnInitPipeline));
            gApi.unregisterAddon(module);
        }
    }
    return TRUE;
}
