#pragma once
// Minimal ReShade Add-on API 18 ABI surface required by the Control FG
// reference-clamp diagnostic. Values/layouts are pinned to ReShade commit
// 4a50d1eddace85734871d91792ff214f13f66c01 (the RenoDX submodule revision).
#include <windows.h>
#include <cstddef>
#include <cstdint>

extern "C" BOOL WINAPI K32EnumProcessModules(HANDLE, HMODULE*, DWORD, LPDWORD);

namespace reshade_min {
inline constexpr uint32_t kApiVersion = 18;
inline constexpr uint32_t kEventInitPipeline = 26;
inline constexpr uint32_t kEventCreatePipeline = 27;

enum class pipeline_subobject_type : uint32_t {
    unknown = 0,
    vertex_shader = 1,
    hull_shader = 2,
    domain_shader = 3,
    geometry_shader = 4,
    pixel_shader = 5,
    compute_shader = 6,
};

struct shader_desc {
    const void* code = nullptr;
    size_t code_size = 0;
    const char* entry_point = nullptr;
    uint32_t spec_constants = 0;
    const uint32_t* spec_constant_ids = nullptr;
    const uint32_t* spec_constant_values = nullptr;
};

struct pipeline_subobject {
    pipeline_subobject_type type = pipeline_subobject_type::unknown;
    uint32_t count = 0;
    void* data = nullptr;
};

struct pipeline_layout { uint64_t handle = 0; };
struct pipeline { uint64_t handle = 0; };
struct device;

using RegisterAddonFn = bool (*)(void*, uint32_t);
using UnregisterAddonFn = void (*)(void*);
using RegisterEventForAddonFn = void (*)(void*, uint32_t, void*);
using UnregisterEventForAddonFn = void (*)(void*, uint32_t, void*);
using LogMessageFn = void (*)(void*, int, const char*);

struct Api {
    HMODULE module = nullptr;
    RegisterAddonFn registerAddon = nullptr;
    UnregisterAddonFn unregisterAddon = nullptr;
    RegisterEventForAddonFn registerEventForAddon = nullptr;
    UnregisterEventForAddonFn unregisterEventForAddon = nullptr;
    LogMessageFn logMessage = nullptr;

    bool valid() const noexcept {
        return module && registerAddon && unregisterAddon && registerEventForAddon && unregisterEventForAddon && logMessage;
    }
};

inline Api FindApi() noexcept {
    HMODULE modules[1024]{};
    DWORD bytes = 0;
    Api out{};
    if (!K32EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &bytes)) return out;
    if (bytes > sizeof(modules)) bytes = sizeof(modules);
    const DWORD count = bytes / sizeof(HMODULE);
    for (DWORD i = 0; i < count; ++i) {
        const auto reg = reinterpret_cast<RegisterAddonFn>(GetProcAddress(modules[i], "ReShadeRegisterAddon"));
        const auto unreg = reinterpret_cast<UnregisterAddonFn>(GetProcAddress(modules[i], "ReShadeUnregisterAddon"));
        const auto regEvent = reinterpret_cast<RegisterEventForAddonFn>(GetProcAddress(modules[i], "ReShadeRegisterEventForAddon"));
        const auto unregEvent = reinterpret_cast<UnregisterEventForAddonFn>(GetProcAddress(modules[i], "ReShadeUnregisterEventForAddon"));
        const auto log = reinterpret_cast<LogMessageFn>(GetProcAddress(modules[i], "ReShadeLogMessage"));
        if (reg && unreg && regEvent && unregEvent && log) {
            out.module = modules[i];
            out.registerAddon = reg;
            out.unregisterAddon = unreg;
            out.registerEventForAddon = regEvent;
            out.unregisterEventForAddon = unregEvent;
            out.logMessage = log;
            break;
        }
    }
    return out;
}
}
