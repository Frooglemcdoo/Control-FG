#pragma once
#include <atomic>
namespace control_visual {
// Persistent preferences. Capture/comparison overrides never write these values.
inline std::atomic<unsigned> characterQuality{3};
inline std::atomic<bool> lightingEnabled{true};
inline std::atomic<unsigned> lightingRevision{1};
inline std::atomic<bool> fpsEnabled{false};
inline void SetLighting(bool value) noexcept {
 lightingEnabled.store(value);lightingRevision.fetch_add(1);
}
}
