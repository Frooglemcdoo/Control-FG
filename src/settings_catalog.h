#pragma once
#include <array>
#include <cstdlib>
#include "settings_schema.generated.h"

// Renderer adapters only. UI metadata and the shared validation contract are
// defined in settings_schema.json. Callers serialize access with fgSettingsMutex.
namespace control_settings {
enum class WriteReason { UserChange, Reset };

inline bool AllowedFG(unsigned value) {
    const unsigned maximum = GetFGMaxSupportedMultiplier();
    return value <= 6 && IsFGSelectionAllowedByGpuPolicy(value) &&
        !(value == 1 && IsFGDynamicCapabilityKnown() && !IsFGDynamicMFGSupported()) &&
        !(value >= 2 && maximum && value > maximum);
}
inline const Option* Find(int id) {
    for (const auto& option : kOptions) if (option.id == id) return &option;
    return nullptr;
}
inline const Option& Definition(Key key) {
    for (const auto& option : kOptions) if (option.key == key) return option;
    // Every Key is generated with an Option; callers never cast external IDs.
    std::abort();
}
inline bool Visible(const Option& option) {
    return option.key != Key::ExperimentalMultiFrameGeneration || IsFGRtx40Series();
}
inline unsigned Default(const Option& option) {
    if (option.key == Key::FrameGeneration && !AllowedFG(option.defaultValue))
        return AllowedFG(2) ? 2 : 0;
    return option.defaultValue;
}
inline unsigned Default(Key key) { return Default(Definition(key)); }
static unsigned manualTarget = Default(Key::DynamicTargetFps);
inline bool Valid(const Option& option, int value) {
    if (value < 0 || !Visible(option)) return false;
    const unsigned requested = static_cast<unsigned>(value);
    if (option.key == Key::FrameGeneration && !AllowedFG(requested)) return false;
    if (option.choices.size()) {
        for (unsigned choice : option.choices) if (requested == choice) return true;
        return false;
    }
    return requested >= option.minimum && requested <= option.maximum &&
        (requested - option.minimum) % option.step == 0;
}
inline unsigned Read(Key key) {
    switch (key) {
    case Key::FrameGeneration: return GetFGUserMultiplier();
    case Key::DynamicTargetMode: return GetFGDynamicManualTargetFps() ? 1 : 0;
    case Key::DynamicTargetFps: {
        const unsigned current = GetFGDynamicManualTargetFps();
        if (current) manualTarget = current;
        return manualTarget;
    }
    case Key::RayReconstruction: return unsigned(control_rr::RRUserRequested());
    case Key::RayReconstructionModel: return control_rr::RRUserPresetValue();
    case Key::ReflectionDetail: return control_rr_clamp::Requested();
    case Key::GlobalIlluminationSamples: return control_rr::GISampleRequested();
    case Key::GlassReflections: return unsigned(PTReflectionUserRequested());
    case Key::TextureStreamingBudget: return GetFGTextureStreamingBudgetMs();
    case Key::VideoMemoryMonitor: return unsigned(IsFGVramMonitorEnabled());
    case Key::ExperimentalMultiFrameGeneration: return unsigned(IsRTX40MFGRequested());
    }
    std::abort(); // A newly added key requires an explicit renderer reader.
}
inline unsigned ReadStored(const Option& option) {
    // Zero is the existing INI representation of Automatic, while the slider
    // keeps the last manual value for the current session.
    if (option.key == Key::DynamicTargetFps) return GetFGDynamicManualTargetFps();
    return Read(option.key);
}
inline std::array<unsigned, kOptionCount> Snapshot(bool defaults = false) {
    std::array<unsigned, kOptionCount> values{};
    for (std::size_t i = 0; i < kOptionCount; ++i)
        values[i] = defaults ? Default(kOptions[i]) : Read(kOptions[i].key);
    return values;
}
inline bool Enabled(const Option& option) {
    if (!Visible(option)) return false;
    for (const auto& dependency : option.dependencies)
        if (Read(dependency.key) != dependency.value) return false;
    return true;
}
inline unsigned RestoreFG(unsigned stored) {
    // Preserve Off and supported saved selections; only coerce unsupported modes.
    return AllowedFG(stored) ? stored : Default(Key::FrameGeneration);
}
inline bool Write(const Option& option, int value, WriteReason reason = WriteReason::UserChange) {
    if (!Valid(option, value)) return false;
    if (reason == WriteReason::UserChange && !Enabled(option)) return false;
    const unsigned requested = static_cast<unsigned>(value);
    switch (option.key) {
    case Key::FrameGeneration: SetFGUserMultiplier(requested); break;
    case Key::DynamicTargetMode: SetFGDynamicManualTargetFps(requested ? manualTarget : 0); break;
    case Key::DynamicTargetFps:
        manualTarget = requested;
        if (reason != WriteReason::Reset) SetFGDynamicManualTargetFps(requested);
        break;
    case Key::RayReconstruction:
        control_rr::RRUserMarkPresetRestartRequired(false);
        control_rr::RRUserRequest(requested != 0);
        break;
    case Key::RayReconstructionModel:
        control_rr::RRUserSetPresetValue(requested);
        control_rr::RRUserMarkPresetRestartRequired(false);
        break;
    case Key::ReflectionDetail: control_rr_clamp::Select(requested); break;
    case Key::GlobalIlluminationSamples: control_rr::GISampleSelect(requested); break;
    case Key::GlassReflections: PTReflectionUserRequest(requested != 0); break;
    case Key::TextureStreamingBudget:
        if (!ApplyTextureStreamingExperimental(requested)) return false;
        SetFGTextureStreamingBudgetMs(requested);
        break;
    case Key::VideoMemoryMonitor: SetFGVramMonitorEnabled(requested != 0); break;
    case Key::ExperimentalMultiFrameGeneration: slRtx40MfgRequested.store(requested); break;
    default: return false; // New schema entries must supply a renderer writer.
    }
    return true;
}
}
